#include "config.hpp"
#include <fstream>
#include <iostream>

AppConfig AppConfig::load(const std::string &filename)
{
    std::ifstream file(filename);
    if (!file.is_open())
    {
        throw std::runtime_error("Cannot open config file: " + filename);
    }

    json j;
    file >> j;

    AppConfig config;

    // Kafka
    config.kafka.bootstrap_servers = j["kafka"]["bootstrap_servers"];
    config.kafka.topics.input = j["kafka"]["topics"]["input"];
    config.kafka.topics.output = j["kafka"]["topics"]["output"];
    config.kafka.topics.errors = j["kafka"]["topics"]["errors"];
    config.kafka.producer.acks = j["kafka"]["producer"]["acks"];
    config.kafka.producer.retries = j["kafka"]["producer"]["retries"];
    config.kafka.producer.batch_size = j["kafka"]["producer"]["batch_size"];
    config.kafka.producer.linger_ms = j["kafka"]["producer"]["linger_ms"];
    config.kafka.consumer.group_id = j["kafka"]["consumer"]["group_id"];
    config.kafka.consumer.auto_offset_reset = j["kafka"]["consumer"]["auto_offset_reset"];
    config.kafka.consumer.enable_auto_commit = j["kafka"]["consumer"]["enable_auto_commit"];

    // Database — загружаем в ConnectionParams
    config.database.postgresql.host = j["database"]["postgresql"]["host"];
    config.database.postgresql.port = j["database"]["postgresql"]["port"];
    config.database.postgresql.database = j["database"]["postgresql"]["database"];
    config.database.postgresql.username = j["database"]["postgresql"]["username"];
    config.database.postgresql.password = j["database"]["postgresql"]["password"];

    // Processing
    config.processing.max_workers = j["processing"]["max_workers"];
    config.processing.batch_size = j["processing"]["batch_size"];
    config.processing.retry_interval_seconds = j["processing"]["retry_interval_seconds"];
    config.processing.delete_after_send = j["processing"]["delete_after_send"];

    // Cache
    config.cache.path = j["cache"]["path"];
    config.cache.retention_days = j["cache"].value("retention_days", 7);
    config.cache.reprocess_delay_seconds = j["cache"].value("reprocess_delay_seconds", 7);
    config.cache.source_prefix = j["cache"].value("source_prefix", "localhost:9092|orders.input");

    // Mode
    config.mode = j.value("mode", "both");

    // Groups
    for (const auto &group : j["groups"])
    {
        GroupConfig g;
        g.name = group["name"];
        g.enabled = group["enabled"];
        g.input_directory = group["input_directory"];
        g.kafka_topic = group["kafka_topic"];
        config.groups.push_back(g);
    }

    return config;
}