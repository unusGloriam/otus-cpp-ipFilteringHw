#pragma once

#include "../database/postgresql.hpp"
#include "../parser/json_parser.hpp"
#include "../cache/sqlite_cache.hpp"
#include "../kafka/producer.hpp"
#include <string>
#include <optional>
#include <vector>

class OrderProcessor
{
public:
    OrderProcessor(database::PostgreSQL &db, MessageCache &cache, KafkaProducer &error_producer);

    // Обработка одного сообщения из Kafka
    bool processMessage(const std::string &key, const std::string &value, int64_t timestamp);

    // Повторная обработка сообщений из кэша
    void reprocessPendingMessages();

private:
    database::PostgreSQL &db_;
    MessageCache &cache_;
    KafkaProducer &error_producer_;

    // Отправка ошибки в топик orders.errors
    void sendError(const std::string &error_type, const std::string &description, const std::string &original_message = "");

    long long getLinuxTime();
    std::optional<std::string> processClient(const OrderData &order);
    std::vector<database::OrderItem> processProducts(const OrderData &order);
    bool createOrder(const OrderData &order, const std::string &client_id,
                     const std::vector<database::OrderItem> &items);
    void logToRegister(const OrderData &order, const std::string &json_str);
    // bool isOrderAlreadyProcessed(const std::string &key);
};