#include "order_processor.hpp"
#include "../utils/logger.hpp"
#include "../utils/uuid.hpp"
#include <chrono>
#include <ctime>
#include <sstream>
#include <nlohmann/json.hpp>

// ============================================================
// Конструктор
// ============================================================
OrderProcessor::OrderProcessor(database::PostgreSQL &db,
                               MessageCache &cache,
                               KafkaProducer &error_producer)
    : db_(db), cache_(cache), error_producer_(error_producer) {}

// ============================================================
// Получение Linux времени (миллисекунды с 1970)
// ============================================================
long long OrderProcessor::getLinuxTime()
{
    auto now = std::chrono::system_clock::now();
    auto duration = now.time_since_epoch();
    return std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();
}

// ============================================================
// Отправка ошибки в топик orders.errors
// ============================================================
void OrderProcessor::sendError(const std::string &error_type,
                               const std::string &description,
                               const std::string &original_message)
{
    try
    {
        std::string uuid = utils::generateUUID(); // Генерация UUID
        std::string key = error_type + " " + uuid;

        nlohmann::json error_json; // Формирование JSON
        error_json["error_type"] = error_type;
        error_json["uuid"] = uuid;
        error_json["timestamp"] = getLinuxTime();
        error_json["description"] = description;
        if (!original_message.empty())
        {
            error_json["original_message"] = original_message;
        }

        std::string error_str = error_json.dump();

        if (error_producer_.send(key, error_str)) // Отправка в Kafka
        {
            Logger::debug("Error sent to Kafka: " + key);
        }
        else
        {
            Logger::error("Failed to send error to Kafka: " + key);
        }
    }
    catch (const std::exception &e)
    {
        Logger::error("sendError failed: " + std::string(e.what()));
    }
}

bool OrderProcessor::isOrderAlreadyProcessed(const std::string &key)
{
    // Проверяем в SQLite, есть ли уже обработанное сообщение с таким ключом
    auto pending = cache_.getPendingMessages(1000);
    for (const auto &[id, topic, message, filename] : pending)
    {
        // Проверяем, содержит ли сообщение этот ключ
        if (message.find(key) != std::string::npos)
        {
            // Нашли сообщение с таким ключом - значит уже обрабатывали
            Logger::warning("Order already in cache: " + key);
            return true;
        }
    }

    // Также можно проверить в PostgreSQL (по номеру заказа и ИНН)
    // Но это отдельный запрос, который может замедлить работу

    return false;
}

// ============================================================
// Обработка одного сообщения из Kafka
// ============================================================
bool OrderProcessor::processMessage(const std::string &key,
                                    const std::string &value,
                                    int64_t timestamp)
{
    Logger::debug("Processing message: " + key);

    try
    {
        // 1. Проверяем не обработан ли уже заказ
        if (isOrderAlreadyProcessed(key))
        {
            Logger::warning("Order already processed, skipping: " + key);
            return true; // Возвращаем true, чтобы не считать ошибкой
        }

        // 2. Проверяем, что это JSON
        if (value.empty() || (value[0] != '{' && value[0] != '['))
        {
            std::string error_msg = "Message is not valid JSON: " + value.substr(0, 100);
            Logger::warning(error_msg);
            sendError("Kafka read error", error_msg, value);
            return false;
        }

        // 3. Парсим JSON
        OrderData order = OrderData::fromJson(value);
        if (!JsonParser::validate(order))
        {
            std::string error_msg = "Invalid order structure: missing TIN, TRRC or goods";
            Logger::error(error_msg);
            sendError("Kafka read error", error_msg, value);
            return false;
        }

        // 4. Сохраняем в SQLite кэш (на случай сбоя БД)
        std::string cache_key = "order_" + order.tin + "_" + order.number;
        std::string source_type = "consumer";
        if (!cache_.save(cache_key, "processing", value, order.tin, order.trrc, source_type))
        {
            Logger::warning("Failed to save to cache, continuing...");
        }

        // 5. Логируем в регистр сведений
        logToRegister(order, value);

        // 6. Обрабатываем контрагента
        auto client_id = processClient(order);
        if (!client_id)
        {
            std::string error_msg = "Client not found and could not be created: TIN=" + order.tin;
            Logger::error(error_msg);
            sendError("PostgreSQL write error", error_msg, value);
            return false;
        }

        // 7. Обрабатываем товары
        auto items = processProducts(order);
        if (items.empty())
        {
            std::string error_msg = "No valid products found in order";
            Logger::error(error_msg);
            sendError("PostgreSQL write error", error_msg, value);
            return false;
        }

        // 8. Создаем заказ
        if (!createOrder(order, *client_id, items))
        {
            std::string error_msg = "Failed to create order in PostgreSQL";
            Logger::error(error_msg);
            sendError("PostgreSQL write error", error_msg, value);
            return false;
        }

        Logger::info("Order processed successfully: " + order.number);
        return true;
    }
    catch (const std::exception &e)
    {
        std::string error_msg = "Exception: " + std::string(e.what());
        Logger::error("processMessage error: " + error_msg);
        sendError("Kafka read error", error_msg, value);
        return false;
    }
}

// ============================================================
// Повторная обработка сообщений из кэша
// ============================================================
void OrderProcessor::reprocessPendingMessages()
{
    auto pending = cache_.getPendingMessages(1000); // Получение pending
    if (pending.empty())
    {
        Logger::info("No pending messages to reprocess");
        return;
    }

    Logger::info("Found " + std::to_string(pending.size()) + " pending messages to reprocess");

    for (const auto &[id, topic, message, filename] : pending) // Цикл по pending
    {
        Logger::info("Reprocessing: " + filename);

        try
        {
            OrderData order = OrderData::fromJson(message); // Парсинг
            if (!JsonParser::validate(order))
            {
                Logger::error("Invalid cached order: " + filename);
                cache_.markError(id, "Invalid JSON in cache");
                sendError("PostgreSQL write error", "Invalid cached order: " + filename, message);
                continue;
            }

            logToRegister(order, message);

            auto client_id = processClient(order);
            if (!client_id)
            {
                Logger::error("Failed to reprocess client: " + order.tin);
                cache_.markError(id, "Client not found");
                sendError("PostgreSQL write error", "Client not found: TIN=" + order.tin, message);
                continue;
            }

            auto items = processProducts(order);
            if (items.empty())
            {
                Logger::error("No valid products in cached order: " + filename);
                cache_.markError(id, "No products found");
                sendError("PostgreSQL write error", "No products found in order", message);
                continue;
            }

            if (!createOrder(order, *client_id, items))
            {
                Logger::error("Failed to reprocess order: " + filename);
                cache_.markError(id, "Order creation failed");
                sendError("PostgreSQL write error", "Order creation failed", message);
                continue;
            }

            cache_.markSent(id); // Обновление статуса
            Logger::info("Reprocessed successfully: " + filename);
        }
        catch (const std::exception &e)
        {
            Logger::error("Reprocess error: " + std::string(e.what()));
            cache_.markError(id, e.what());
            sendError("PostgreSQL write error", "Reprocess error: " + std::string(e.what()), message);
        }
    }
}

// ============================================================
// Обработка контрагента (поиск или создание)
// ============================================================
std::optional<std::string> OrderProcessor::processClient(const OrderData &order)
{
    Logger::info("processClient: TIN=" + order.tin + ", TRRC=" + order.trrc);

    auto client = db_.findClient(order.tin, order.trrc);

    if (client)
    {
        Logger::info("Client found: " + client->id);
        return client->id;
    }

    // ★ НЕ НАЙДЕН - СОЗДАЕМ ★
    Logger::info("Client NOT found, creating new client...");
    std::string name = order.contractor.empty() ? "Контрагент " + order.tin : order.contractor;
    std::string client_id = db_.createClient(order.tin, order.trrc, name);

    if (client_id.empty())
    {
        Logger::error("Failed to create client: " + order.tin);
        return std::nullopt;
    }

    Logger::info("Client created: " + client_id);
    return client_id;
}

// ============================================================
// Обработка товаров (поиск по артикулам)
// ============================================================
std::vector<database::OrderItem> OrderProcessor::processProducts(const OrderData &order)
{
    std::vector<database::OrderItem> items;

    for (const auto &item : order.goods) // Цикл по товарам
    {
        auto product = db_.findProductByArticle(item.sku);

        if (!product) // Товар не найден
        {
            Logger::warning("Product not found: " + item.sku);
            continue;
        }

        database::OrderItem db_item;
        db_item.product_id = product->id;
        db_item.quantity = item.quantity;
        db_item.price = item.price;
        db_item.sum = item.sum;

        items.push_back(db_item); // Добавление товара
        Logger::debug("Product added: " + item.sku + " (x" + std::to_string(item.quantity) + ")");
    }

    return items;
}

// ============================================================
// Создание заказа
// ============================================================
bool OrderProcessor::createOrder(const OrderData &order,
                                 const std::string &client_id,
                                 const std::vector<database::OrderItem> &items)
{
    if (items.empty()) // Проверка товаров
    {
        Logger::error("Cannot create order with empty items");
        return false;
    }

    std::string order_number = order.number;
    if (order_number.empty())
    {
        auto now = std::chrono::system_clock::now();
        auto time_t = std::chrono::system_clock::to_time_t(now);
        order_number = "AUTO-" + std::to_string(time_t); // Генерация номера
    }

    std::string order_id = db_.createOrder( // Создание в БД
        client_id,
        order.date,
        order_number,
        items);

    return !order_id.empty();
}

// ============================================================
// Логирование в регистр сведений
// ============================================================
void OrderProcessor::logToRegister(const OrderData &order, const std::string &json_str)
{
    long long linux_time = getLinuxTime();
    db_.logKafkaMessage(linux_time, order.tin, order.trrc, json_str);
    Logger::debug("Logged to register: " + order.tin);
}