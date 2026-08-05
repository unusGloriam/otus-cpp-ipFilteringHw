#pragma once

#include <librdkafka/rdkafkacpp.h>
#include <string>
#include <memory>
#include <functional>
#include <atomic>
#include <thread>

class KafkaConsumer
{
public:
    using MessageCallback = std::function<void(const std::string &key,
                                               const std::string &value,
                                               int64_t timestamp)>;

    KafkaConsumer(const std::string &brokers, const std::string &group_id, const std::string &topic);
    ~KafkaConsumer();

    bool init();
    void start();
    void stop();
    void setMessageCallback(MessageCallback cb);

    bool isRunning() const { return running_.load(); }

private:
    void consumeLoop();

    std::string brokers_;
    std::string group_id_;
    std::string topic_;
    std::unique_ptr<RdKafka::KafkaConsumer> consumer_;
    MessageCallback callback_;
    std::atomic<bool> running_;
    std::unique_ptr<std::thread> consumer_thread_;
    std::string errstr_;
};