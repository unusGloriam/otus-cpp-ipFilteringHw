#include "postgresql.hpp"
#include "../utils/logger.hpp"
#include <sstream>
#include <iomanip>
#include <chrono>
#include <random>
#include <ctime>
#include <algorithm>

namespace database
{

    // ============================================================
    // Конструкторы / Деструктор
    // ============================================================

    PostgreSQL::PostgreSQL()
        : conn_(nullptr), connected_(false) {}

    PostgreSQL::PostgreSQL(const ConnectionParams &params)
        : params_(params), conn_(nullptr), connected_(false) {}

    PostgreSQL::~PostgreSQL()
    {
        disconnect();
    }

    // ============================================================
    // Подключение к БД
    // ============================================================

    bool PostgreSQL::connect()
    {
        try
        {
            if (params_.host.empty())
            {
                conn_ = std::make_unique<pqxx::connection>("");
            }
            else
            {
                conn_ = std::make_unique<pqxx::connection>(params_.connectionString());
            }

            if (!conn_->is_open())
            {
                Logger::error("Failed to connect to PostgreSQL");
                return false;
            }

            // УСТАНАВЛИВАЕМ КОДИРОВКУ
            {
                pqxx::work txn(*conn_);

                // Проверяем кодировку
                pqxx::result res = txn.exec("SHOW client_encoding;");
                std::string encoding = res[0][0].as<std::string>();
                Logger::info("PostgreSQL client_encoding: " + encoding);

                txn.exec("SET client_encoding = 'UTF8';");
                txn.exec("SET standard_conforming_strings = on;");
                txn.commit();
            }

            connected_ = true;
            Logger::info("Connected to PostgreSQL");
            return true;
        }
        catch (const std::exception &e)
        {
            Logger::error("PostgreSQL connection error: " + std::string(e.what()));
            return false;
        }
    }

    bool PostgreSQL::isConnected() const
    {
        return connected_ && conn_ && conn_->is_open();
    }

    void PostgreSQL::disconnect()
    {
        //if (conn_ && conn_->is_open())
        //{
        //    conn_->close();
        //}
        conn_.reset();
        connected_ = false;
    }

    // ============================================================
    // Выполнение SQL запросов
    // ============================================================

    bool PostgreSQL::execute(const std::string &sql)
    {
        if (!isConnected())
        {
            Logger::error("Not connected to PostgreSQL");
            return false;
        }

        try
        {
            pqxx::work txn(*conn_);
            txn.exec(sql);
            txn.commit();
            return true;
        }
        catch (const std::exception &e)
        {
            Logger::error("SQL error: " + std::string(e.what()));
            Logger::error("SQL: " + sql);
            return false;
        }
    }

    bool PostgreSQL::executeParams(const std::string &sql, const std::vector<std::string> &params)
    {
        if (!isConnected())
        {
            Logger::error("Not connected to PostgreSQL");
            return false;
        }

        try
        {
            pqxx::work txn(*conn_);

            std::string query = sql;
            for (size_t i = 0; i < params.size(); ++i)
            {
                std::string placeholder = "$" + std::to_string(i + 1);
                size_t pos = query.find(placeholder);
                if (pos != std::string::npos)
                {
                    query.replace(pos, placeholder.length(), "'" + escape(params[i]) + "'");
                }
            }

            txn.exec(query);
            txn.commit();
            return true;
        }
        catch (const std::exception &e)
        {
            Logger::error("SQL error: " + std::string(e.what()));
            Logger::error("SQL: " + sql);
            return false;
        }
    }

    pqxx::result PostgreSQL::query(const std::string &sql)
    {
        if (!isConnected())
        {
            Logger::error("Not connected to PostgreSQL");
            return pqxx::result();
        }

        try
        {
            pqxx::work txn(*conn_);
            pqxx::result result = txn.exec(sql);
            txn.commit();
            return result;
        }
        catch (const std::exception &e)
        {
            Logger::error("SQL error: " + std::string(e.what()));
            Logger::error("SQL: " + sql);
            return pqxx::result();
        }
    }

    // ============================================================
    // Вспомогательные функции
    // ============================================================

    std::string PostgreSQL::escape(const std::string &str)
    {
        Logger::debug("escape input: " + str);

        std::string result = str;
        size_t pos = 0;
        while ((pos = result.find("'", pos)) != std::string::npos)
        {
            result.replace(pos, 1, "''");
            pos += 2;
        }
        return result;
    }

    std::string PostgreSQL::generateUUID()
    {
        std::random_device rd;
        std::mt19937_64 gen(rd());
        std::uniform_int_distribution<int> dist(0, 15);

        std::stringstream ss;
        ss << std::hex << std::setfill('0');
        for (int i = 0; i < 36; ++i)
        {
            if (i == 8 || i == 13 || i == 18 || i == 23)
            {
                ss << '-';
            }
            else
            {
                ss << dist(gen);
            }
        }
        return ss.str();
    }

    // ============================================================
    // UUID → HEX (с обратным слешем)
    // ============================================================

    std::string PostgreSQL::convertToHex(const std::string &uuid)
    {
        if (uuid.length() != 36)
        {
            return uuid;
        }

        // Удаляем дефисы
        std::string clean = uuid;
        clean.erase(std::remove(clean.begin(), clean.end(), '-'), clean.end());

        // Добавляем \x в начало (экранируем слэш для C++)
        return "\\x" + clean;
    }

    // ============================================================
    // ★ HEX → UUID (с дефисами, перевернутый) ★
    // ============================================================

    std::string PostgreSQL::convertTo1CUUID(const std::string &hex)
    {
        // Если это HEX с \x - убираем префикс
        std::string clean = hex;
        if (clean.find("\\x") == 0)
        {
            clean = clean.substr(2);
        }
        if (clean.find("0x") == 0)
        {
            clean = clean.substr(2);
        }

        if (clean.length() != 32)
        {
            return hex;
        }

        // Разбиваем на группы: a362345a60c3974211f18e88975e67cc
        // a362345a | 60c3 | 9742 | 11f1 | 8e88975e67cc
        std::string part1 = clean.substr(0, 8);
        std::string part2 = clean.substr(8, 4);
        std::string part3 = clean.substr(12, 4);
        std::string part4 = clean.substr(16, 4);
        std::string part5 = clean.substr(20, 12);

        // Переворачиваем первые 3 группы
        std::string reversed =
            part1.substr(6, 2) + part1.substr(4, 2) + part1.substr(2, 2) + part1.substr(0, 2) + "-" +
            part2.substr(2, 2) + part2.substr(0, 2) + "-" +
            part3.substr(2, 2) + part3.substr(0, 2) + "-" +
            part4 + "-" +
            part5;

        return reversed;
    }

    std::string PostgreSQL::formatDate(const std::string &date)
    {
        if (date.find('T') != std::string::npos)
        {
            std::string result = date;
            std::replace(result.begin(), result.end(), 'T', ' ');
            if (result.find('.') == std::string::npos)
            {
                result += ".000";
            }
            return result;
        }

        auto now = std::chrono::system_clock::now();
        std::time_t now_time = std::chrono::system_clock::to_time_t(now);
        std::tm tm;
#ifdef _WIN32
        localtime_s(&tm, &now_time);
#else
        localtime_r(&now_time, &tm);
#endif

        std::stringstream ss;
        ss << std::put_time(&tm, "%Y-%m-%d %H:%M:%S.000");
        return ss.str();
    }

    // ============================================================
    // Поиск контрагента
    // ============================================================

    std::optional<Client> PostgreSQL::findClient(const std::string &inn, const std::string &kpp)
    {
        try
        {
            std::string sql;

            if (kpp.empty())
            {
                sql = R"(
                SELECT _IDRRef, _Description, _Fld50, _Fld51, _Marked
                FROM _Reference47
                WHERE _Fld50 = ')" +
                      escape(inn) +
                      R"('
                AND (_Fld51 = '' OR _Fld51 IS NULL)
                AND _Marked = FALSE
                )";
            }
            else
            {
                sql = R"(
                SELECT _IDRRef, _Description, _Fld50, _Fld51, _Marked
                FROM _Reference47
                WHERE _Fld50 = ')" +
                      escape(inn) +
                      R"('
                AND _Fld51 = ')" +
                      escape(kpp) +
                      R"('
                AND _Marked = FALSE
                )";
            }

            Logger::debug("findClient SQL: " + sql);

            pqxx::result result = query(sql);
            if (result.empty())
            {
                Logger::warning("Client not found: INN=" + inn + ", KPP=" + kpp);
                return std::nullopt;
            }

            Client client;
            for (const auto &row : result)
            {
                client.id = row[0].as<std::string>(); // УЖЕ В HEX ФОРМАТЕ (\x...)
                client.name = row[1].as<std::string>();
                client.inn = row[2].as<std::string>();
                client.kpp = row[3].as<std::string>();
                client.marked = row[4].as<bool>();
                break;
            }

            Logger::info("Client found: " + client.inn + " (" + client.name + ")");
            return client;
        }
        catch (const std::exception &e)
        {
            Logger::error("findClient error: " + std::string(e.what()));
            return std::nullopt;
        }
    }

    // ============================================================
    // Создание контрагента
    // ============================================================

    std::string PostgreSQL::createClient(const std::string &inn, const std::string &kpp, const std::string &name)
    {
        try
        {

            // ОБРЕЗАЕМ ИМЯ ДО 25 СИМВОЛОВ ТАК КАК ДЛИНА НАИМЕНОВАНИЯ 25 СИМВОЛОВ
            std::string short_name = name;
            if (short_name.length() > 25)
            {
                short_name = short_name.substr(0, 25);
                Logger::warning("Name truncated to 25 chars: " + name + " -> " + short_name);
            }

            std::string id = generateUUID();
            std::string id_hex = convertToHex(id); // HEX с \x

            // ДОПОЛНЯЕМ _Code ДО 9 СИМВОЛОВ
            std::string code = inn.substr(0, 8);
            while (code.length() < 9)
            {
                code = "0" + code;
            }

            Logger::debug("Creating client with name: " + name);

            // НУЛЕВОЙ UUID ДЛЯ _PredefinedID
            std::string empty_uuid = "00000000-0000-0000-0000-000000000000";
            std::string empty_hex = convertToHex(empty_uuid); // ★ HEX с \x ★

            std::string sql = R"(
            INSERT INTO _Reference47 (_IDRRef, _Code, _Description, _Fld50, _Fld51, _Marked, _PredefinedID)
            VALUES (')" + id_hex +
                              R"(', ')" + code +
                              R"(', ')" + escape(name) +
                              R"(', ')" + escape(inn) +
                              R"(', ')" + escape(kpp) +
                              R"(', FALSE, ')" + empty_hex + R"(')
            )";

            Logger::debug("createClient SQL: " + sql);

            if (!execute(sql))
            {
                Logger::error("Failed to create client: " + inn);
                return "";
            }

            Logger::info("Created client: " + inn + " (" + name + "), ID: " + id);
            return id_hex; // ВОЗВРАЩАЕМ HEX
        }
        catch (const std::exception &e)
        {
            Logger::error("createClient error: " + std::string(e.what()));
            return "";
        }
    }

    // ============================================================
    // Поиск товара по артикулу
    // ============================================================

    std::optional<Product> PostgreSQL::findProductByArticle(const std::string &article)
    {
        try
        {
            std::string sql = R"(
            SELECT _IDRRef, _Code, _Description, _Fld52, _Marked
            FROM _Reference48
            WHERE _Fld52 = ')" +
                              escape(article) +
                              R"('
            AND _Marked = FALSE
            )";

            Logger::debug("findProductByArticle SQL: " + sql);

            pqxx::result result = query(sql);
            if (result.empty())
            {
                Logger::warning("Product not found: article=" + article);
                return std::nullopt;
            }

            Product product;
            for (const auto &row : result)
            {
                product.id = row[0].as<std::string>(); // УЖЕ В HEX ФОРМАТЕ (\x...)
                product.code = row[1].as<std::string>();
                product.name = row[2].as<std::string>();
                product.article = row[3].as<std::string>();
                product.marked = row[4].as<bool>();
                break;
            }

            Logger::info("Product found: " + product.article + " (" + product.name + ")");
            return product;
        }
        catch (const std::exception &e)
        {
            Logger::error("findProductByArticle error: " + std::string(e.what()));
            return std::nullopt;
        }
    }

    // ============================================================
    // Создание заказа
    // ============================================================

    std::string PostgreSQL::createOrder(
        const std::string &client_id,
        const std::string &date,
        const std::string &number,
        const std::vector<OrderItem> &items)
    {
        try
        {
            if (items.empty())
            {
                Logger::error("Cannot create order with empty items");
                return "";
            }

            if (!isConnected())
            {
                Logger::error("Not connected to PostgreSQL");
                return "";
            }

            std::string order_id = generateUUID();
            std::string order_id_hex = convertToHex(order_id); // ★ HEX с \x ★
            std::string order_date = formatDate(date);
            std::string order_number = number.empty() ? "AUTO-" + std::to_string(std::time(nullptr)) : number;

            // ТРАНЗАКЦИЯ
            pqxx::work txn(*conn_);

            // 1. Создаем документ (шапку заказа)
            std::string sql_order = R"(
            INSERT INTO _Document49 (_IDRRef, _Date_Time, _Number, _Posted, _Marked, _Fld53RRef)
            VALUES (')" + order_id_hex +
                                    R"(', ')" + order_date +
                                    R"(', ')" + escape(order_number) +
                                    R"(', TRUE, FALSE, ')" + escape(client_id) + R"(')
            )";

            Logger::debug("createOrder - header SQL: " + sql_order);
            txn.exec(sql_order);
            Logger::info("Created order header: " + order_number);

            // 2. Создаем строки товаров
            int lineNo = 1;
            for (const auto &item : items)
            {
                // НЕ КОНВЕРТИРУЕМ - product_id УЖЕ В HEX ФОРМАТЕ
                std::string product_id_hex = item.product_id;

                std::string sql_item = R"(
                INSERT INTO _Document49_VT54 (_Document49_IDRRef, _Fld56RRef, _Fld57, _Fld58, _Fld59, _KeyField, _LineNo55)
                VALUES (')" + order_id_hex +
                                       R"(', ')" + escape(product_id_hex) +
                                       R"(', )" + std::to_string(item.quantity) +
                                       R"(, )" + std::to_string(item.price) +
                                       R"(, )" + std::to_string(item.sum) +
                                       R"(, ')" + order_id_hex +
                                       R"(', )" + std::to_string(lineNo) + R"()
                )";

                Logger::debug("createOrder - item SQL: " + sql_item);
                txn.exec(sql_item);
                lineNo++;
            }

            // ФИКСИРУЕМ ТРАНЗАКЦИЮ
            txn.commit();

            Logger::info("Order " + order_number + " created with " + std::to_string(items.size()) + " items");
            return order_id_hex; // ВОЗВРАЩАЕМ HEX
        }
        catch (const std::exception &e)
        {
            Logger::error("createOrder error: " + std::string(e.what()));
            return "";
        }
    }

    // ============================================================
    // Запись в регистр сведений
    // ============================================================

    void PostgreSQL::logKafkaMessage(
        long long linux_time,
        const std::string &tin,
        const std::string &trrc,
        const std::string &json_data)
    {
        try
        {
            std::string sql = R"(
            INSERT INTO _InfoRg60 (_Fld61, _Fld62, _Fld63, _Fld64)
            VALUES ()" + std::to_string(linux_time) +
                              R"(, ')" + escape(tin) +
                              R"(', ')" + escape(trrc) +
                              R"(', ')" + escape(json_data) + R"(')
            )";

            Logger::debug("logKafkaMessage SQL: " + sql);

            execute(sql);
            Logger::debug("Logged Kafka message for TIN: " + tin);
        }
        catch (const std::exception &e)
        {
            Logger::error("logKafkaMessage error: " + std::string(e.what()));
        }
    }

} // namespace database