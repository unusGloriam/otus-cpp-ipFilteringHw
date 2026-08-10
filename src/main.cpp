#include <iostream>
#include <string>
#include <thread>
#include <vector>
#include <atomic>
#include <chrono>
#include <filesystem>
#include <csignal>
#include <iomanip>

#include "config/config.hpp"
#include "kafka/producer.hpp"
#include "kafka/consumer.hpp"
#include "parser/json_parser.hpp"
#include "cache/sqlite_cache.hpp"
#include "database/postgresql.hpp"
#include "processor/order_processor.hpp"
#include "utils/logger.hpp"

namespace fs = std::filesystem;

// ============================================================
// ГЛОБАЛЬНЫЕ ПЕРЕМЕННЫЕ
// ============================================================

std::atomic<bool> running{true};
std::atomic<size_t> producer_count{0};
std::atomic<size_t> consumer_count{0};
std::atomic<size_t> error_count{0};

// ============================================================
// ОБРАБОТЧИК СИГНАЛОВ (Ctrl+C)
// ============================================================

void signalHandler(int signal)
{
    Logger::info("Received signal " + std::to_string(signal) + ", shutting down...");
    running = false;
}

// ============================================================
// ВСПОМОГАТЕЛЬНАЯ ФУНКЦИЯ — ПОЛУЧЕНИЕ СПИСКА ФАЙЛОВ
// ============================================================

std::vector<std::string> getJsonFiles(const std::string &directory)
{
    std::vector<std::string> files;
    try
    {
        for (const auto &entry : fs::directory_iterator(directory))
        {
            if (entry.path().extension() == ".json")
            {
                files.push_back(entry.path().string());
            }
        }
    }
    catch (const std::exception &e)
    {
        Logger::error("Directory read error: " + std::string(e.what()));
    }
    return files;
}

// ============================================================
// ФУНКЦИЯ PRODUCER (отправка файлов в Kafka)
// ============================================================

void processFiles(const std::string &directory,
                  KafkaProducer &producer,
                  MessageCache &cache,
                  const std::string &topic,
                  std::atomic<size_t> &next_index,
                  const std::vector<std::string> &files)
{

    size_t idx;
    while ((idx = next_index.fetch_add(1)) < files.size()) // 1. Lock-free атомарное разделение
    {
        const std::string &filepath = files[idx];

        try
        {
            Logger::debug("[Producer] Processing: " + filepath);

            auto order = JsonParser::parseOrder(filepath); // 2. Начало чтения файла
            if (!JsonParser::validate(order))
            {
                Logger::error("[Producer] Invalid order: " + filepath);
                error_count.fetch_add(1);
                continue;
            }

            std::string json_message = order.toJson();
            std::string key = order.tin + "-" + order.number;
            std::string source_type = "producer";

            if (!cache.save(filepath, topic, json_message, order.tin, order.trrc, source_type)) // 5. Сохранение в SQLite (Producer)
            {
                Logger::error("[Producer] Failed to cache: " + filepath);
                error_count.fetch_add(1);
                continue;
            }

            if (producer.send(key, json_message)) // 8.Отправка в Kafka
            {
                producer_count.fetch_add(1);
                Logger::info("[Producer] Sent: " + key + " (" + std::to_string(producer_count.load()) + ")");

                try
                {
                    fs::remove(filepath);
                    Logger::debug("[Producer] Deleted: " + filepath);
                }
                catch (const std::exception &e)
                {
                    Logger::warning("[Producer] Failed to delete: " + filepath + " - " + e.what());
                }
            }
            else
            {
                Logger::error("[Producer] Failed to send: " + key);
                error_count.fetch_add(1);
            }
        }
        catch (const std::exception &e)
        {
            Logger::error("[Producer] Error processing " + filepath + ": " + e.what());
            error_count.fetch_add(1);
        }
    }
}

// ============================================================
// ФУНКЦИЯ PRODUCER — ЗАПУСК В ОТДЕЛЬНОМ ПОТОКЕ
// ============================================================

void runProducer(AppConfig &config, KafkaProducer &producer, MessageCache &cache)
{
    Logger::info("[Producer] Thread started");

    const int MAX_WORKERS = config.processing.max_workers;
    std::vector<std::thread> workers;

    while (running)
    {
        for (const auto &group : config.groups)
        {
            if (!group.enabled)
                continue;

            auto files = getJsonFiles(group.input_directory);
            if (files.empty())
            {
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
                continue;
            }

            Logger::info("[Producer] Found " + std::to_string(files.size()) + " files in " + group.input_directory);

            std::atomic<size_t> next_index{0};

            for (int i = 0; i < MAX_WORKERS && running; ++i)
            {
                workers.emplace_back([&]()
                                     { processFiles(
                                           group.input_directory,
                                           producer,
                                           cache,
                                           group.kafka_topic,
                                           std::ref(next_index),
                                           std::cref(files)); });
            }

            for (auto &worker : workers)
            {
                if (worker.joinable())
                {
                    worker.join();
                }
            }
            workers.clear();
        }

        if (running)
        {
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
    }

    Logger::info("[Producer] Thread stopped");
}

// ============================================================
// ФУНКЦИЯ CONSUMER — ЗАПУСК В ОТДЕЛЬНОМ ПОТОКЕ
// ============================================================

void runConsumer(AppConfig &config, database::PostgreSQL &db, MessageCache &cache, KafkaProducer &error_producer)
{
    Logger::info("[Consumer] Thread started");

    try
    {
        KafkaConsumer consumer( // Создание consumer
            config.kafka.bootstrap_servers,
            config.kafka.consumer.group_id,
            config.kafka.topics.input);

        if (!consumer.init())
        { // Инициализация
            Logger::error("[Consumer] Failed to initialize");
            return;
        }

        OrderProcessor processor(db, cache, error_producer); // Создание processor

        Logger::info("[Consumer] Checking for pending messages in cache...");
        processor.reprocessPendingMessages(); // Восстановление

        // Устанавливаем callback для обработки сообщений
        consumer.setMessageCallback([&processor](const std::string &key,
                                                 const std::string &value,
                                                 int64_t timestamp)
                                    {
            if (processor.processMessage(key, value, timestamp)) {
                consumer_count.fetch_add(1);
            } else {
                error_count.fetch_add(1);
            } });

        // Запускаем consumer (блокирующий вызов в отдельном потоке)
        consumer.start(); // Запуск consumer

        // Ждем завершения (while running)
        while (running)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }

        // Останавливаем consumer (он автоматически закоммитит offset)
        consumer.stop();
    }
    catch (const std::exception &e)
    {
        Logger::error("[Consumer] Error: " + std::string(e.what()));
    }

    Logger::info("[Consumer] Thread stopped");
}

// ============================================================
// ГЛАВНАЯ ФУНКЦИЯ
// ============================================================

int main(int argc, char *argv[])
{
    // 1. Устанавливаем обработчик сигналов
    signal(SIGINT, signalHandler);
    signal(SIGTERM, signalHandler);

    try
    {
        Logger::info("=== Kafka-1C Connector (Producer + Consumer) ===");
        Logger::info("Version: 1.0.0");

        // 2. Загружаем конфигурацию
        AppConfig config = AppConfig::load("./config/settings.json");
        Logger::info("Config loaded successfully");

        // 3. Инициализируем SQLite кэш
        MessageCache cache(config.cache.path);
        if (!cache.init())
        {
            Logger::error("Failed to initialize cache");
            return 1;
        }

        // УСТАНАВЛИВАЕМ НАСТРОЙКИ
        cache.setSourcePrefix(config.cache.source_prefix);
        cache.setReprocessDelay(config.cache.reprocess_delay_seconds);

        Logger::info("Cache initialized: " + config.cache.path);

        // 4. Инициализируем PostgreSQL
        database::PostgreSQL db(config.database.postgresql);
        if (!db.connect())
        {
            Logger::error("Failed to connect to PostgreSQL");
            return 1;
        }
        Logger::info("PostgreSQL connected: " + config.database.postgresql.database);

        // 5. Инициализируем Kafka Producer (основной)
        KafkaProducer producer(
            config.kafka.bootstrap_servers,
            config.kafka.topics.input);

        if (!producer.init(config.kafka.producer.acks, config.kafka.producer.retries))
        {
            Logger::error("Failed to initialize Kafka producer");
            return 1;
        }

        // 6. Инициализируем Kafka Producer для ошибок
        KafkaProducer error_producer(
            config.kafka.bootstrap_servers,
            config.kafka.topics.errors);

        if (!error_producer.init(config.kafka.producer.acks, config.kafka.producer.retries))
        {
            Logger::error("Failed to initialize Kafka error producer");
            return 1;
        }

        // 7. Устанавливаем callback для подтверждения доставки
        producer.setDeliveryCallback([](const std::string &key, int error, int64_t offset)
                                     {
            if (error == 0) {
                Logger::debug("[Producer] Delivered: " + key + " (offset: " + std::to_string(offset) + ")");
            } else {
                Logger::warning("[Producer] Delivery failed: " + key + " (error: " + std::to_string(error) + ")");
            } });

        // 8. Определяем режим работы
        std::string mode = config.mode;
        if (argc > 1)
        {
            mode = argv[1];
        }

        Logger::info("Mode: " + mode);

        // 9. Запускаем потоки в зависимости от режима
        std::thread producer_thread;
        std::thread consumer_thread;

        if (mode == "producer" || mode == "both")
        {
            Logger::info("Starting Producer thread...");
            producer_thread = std::thread(runProducer, std::ref(config), std::ref(producer), std::ref(cache));
        }

        if (mode == "consumer" || mode == "both")
        {
            Logger::info("Starting Consumer thread...");
            std::this_thread::sleep_for(std::chrono::seconds(2));
            consumer_thread = std::thread(runConsumer, std::ref(config), std::ref(db), std::ref(cache), std::ref(error_producer));
        }

        // 10. Ждем завершения (Ctrl+C)
        while (running)
        {
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }

        // 11. Остановка потоков
        Logger::info("Stopping threads...");

        if (producer_thread.joinable())
        {
            producer_thread.join();
        }

        if (consumer_thread.joinable())
        {
            consumer_thread.join();
        }

        // 12. Завершение
        producer.flush();
        error_producer.flush();
        cache.cleanup(7);

        Logger::info("=== Summary ===");
        Logger::info("Producer sent: " + std::to_string(producer_count.load()));
        Logger::info("Consumer processed: " + std::to_string(consumer_count.load()));
        Logger::info("Errors: " + std::to_string(error_count.load()));
        Logger::info("Pending in cache: " + std::to_string(cache.getPendingCount()));
        Logger::info("Total in cache: " + std::to_string(cache.getTotalCount()));
        Logger::info("Shutdown complete");
    }
    catch (const std::exception &e)
    {
        Logger::error("Fatal error: " + std::string(e.what()));
        return 1;
    }

    return 0;
}