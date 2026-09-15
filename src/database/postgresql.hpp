#pragma once

#include <pqxx/pqxx>
#include <string>
#include <memory>
#include <optional>
#include <vector>

namespace database
{

    struct ConnectionParams
    {
        std::string host;
        int port;
        std::string database;
        std::string username;
        std::string password;

        std::string connectionString() const
        {
            return "host=" + host +
                   " port=" + std::to_string(port) +
                   " dbname=" + database +
                   " user=" + username +
                   " password=" + password;
        }
    };

    // ============================================================
    // Структуры для работы с 1С таблицами
    // ============================================================

    struct Client
    {
        std::string id;   // _IDRRef (HEX с \x)
        std::string inn;  // _Fld50
        std::string kpp;  // _Fld51
        std::string name; // _Description
        bool marked;      // _Marked
    };

    struct Product
    {
        std::string id;      // _IDRRef (HEX с \x)
        std::string code;    // _Code
        std::string name;    // _Description
        std::string article; // _Fld52
        bool marked;         // _Marked
    };

    struct OrderItem
    {
        std::string product_id; // _Fld56RRef (HEX с \x)
        double quantity;        // _Fld57
        double price;           // _Fld58
        double sum;             // _Fld59
    };

    // ============================================================
    // Класс для работы с PostgreSQL через libpqxx
    // ============================================================

    class PostgreSQL
    {
    public:
        PostgreSQL();
        PostgreSQL(const ConnectionParams &params);
        ~PostgreSQL();

        bool connect();
        bool isConnected() const;
        void disconnect();

        // Выполнение SQL запроса (без результата)
        bool execute(const std::string &sql);

        // Выполнение запроса с параметрами
        bool executeParams(const std::string &sql, const std::vector<std::string> &params);

        // Выполнение запроса с результатом
        pqxx::result query(const std::string &sql);

        // ============================================================
        // Работа с контрагентами
        // ============================================================
        std::optional<Client> findClient(const std::string &inn, const std::string &kpp = "");
        std::string createClient(const std::string &inn, const std::string &kpp, const std::string &name);

        // ============================================================
        // Работа с номенклатурой
        // ============================================================
        std::optional<Product> findProductByArticle(const std::string &article);

        // ============================================================
        // Работа с заказами
        // ============================================================
        std::string createOrder(
            const std::string &client_id,
            const std::string &date,
            const std::string &number,
            const std::vector<OrderItem> &items);

        // Проверка существования заказа
        bool orderExists(const std::string &number, const std::string &client_id);

        // ============================================================
        // Запись в регистр сведений
        // ============================================================
        void logKafkaMessage(
            long long linux_time,
            const std::string &tin,
            const std::string &trrc,
            const std::string &json_data);

    private:
        ConnectionParams params_;
        std::unique_ptr<pqxx::connection> conn_;
        bool connected_;

        std::string escape(const std::string &str);
        std::string generateUUID();
        std::string convertToHex(const std::string &uuid);   // UUID → HEX с \x
        std::string convertTo1CUUID(const std::string &hex); // HEX → UUID с дефисами
        std::string formatDate(const std::string &date);
    };

} // namespace database