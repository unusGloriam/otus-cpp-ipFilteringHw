#pragma once

#include <string>
#include <vector>
#include <nlohmann/json.hpp>
#include "../database/postgresql.hpp" // <-- ДОБАВЛЯЕМ

using json = nlohmann::json;

// ============================================================
// KafkaConfig
// ============================================================
struct KafkaConfig
{
    std::string bootstrap_servers;
    struct Topics
    {
        std::string input;
        std::string output;
        std::string errors;
    } topics;
    struct Producer
    {
        std::string acks;
        int retries;
        int batch_size;
        int linger_ms;
    } producer;
    struct Consumer
    {
        std::string group_id;
        std::string auto_offset_reset;
        bool enable_auto_commit;
    } consumer;
};

// ============================================================
// DatabaseConfig — используем ConnectionParams из database
// ============================================================
struct DatabaseConfig
{
    database::ConnectionParams postgresql;
};

// ============================================================
// ProcessingConfig
// ============================================================
struct ProcessingConfig
{
    int max_workers;
    int batch_size;
    int retry_interval_seconds;
    bool delete_after_send;
};

// ============================================================
// CacheConfig
// ============================================================
struct CacheConfig
{
    std::string path;
    int retention_days = 7;
    int reprocess_delay_seconds = 7;
    std::string source_prefix; // УНИКАЛЬНЫЙ ПРЕФИКС РАЗДЕЛЕНИЯ ПОТОКОВ В КЭШЕ SQLLITE
};

// ============================================================
// GroupConfig
// ============================================================
struct GroupConfig
{
    std::string name;
    bool enabled;
    std::string input_directory;
    std::string kafka_topic;
};

// ============================================================
// AppConfig
// ============================================================
struct AppConfig
{
    KafkaConfig kafka;
    DatabaseConfig database;
    ProcessingConfig processing;
    CacheConfig cache;
    std::vector<GroupConfig> groups;
    std::string mode; // "producer", "consumer", "both"

    static AppConfig load(const std::string &filename);
};