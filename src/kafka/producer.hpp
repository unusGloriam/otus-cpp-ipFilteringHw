#pragma once

#include <librdkafka/rdkafkacpp.h>
#include <string>
#include <memory>
#include <functional>
#include <atomic>

class KafkaProducer {
public:
    using DeliveryCallback = std::function<void(const std::string&, int, int64_t)>;

    KafkaProducer(const std::string& brokers, const std::string& topic);
    ~KafkaProducer();

    bool init(const std::string& acks = "all", int retries = 3);
    bool send(const std::string& message);
    bool send(const std::string& key, const std::string& message);
    void setDeliveryCallback(DeliveryCallback cb);
    void flush(int timeout_ms = 5000);
    
    // Статистика
    size_t getSentCount() const { return sent_count_.load(); }
    size_t getFailedCount() const { return failed_count_.load(); }

private:
    class DeliveryReportCb : public RdKafka::DeliveryReportCb {
    public:
        DeliveryReportCb(KafkaProducer* producer);
        void dr_cb(RdKafka::Message& msg) override;
    private:
        KafkaProducer* producer_;
    };

    std::string brokers_;
    std::string topic_;
    std::unique_ptr<RdKafka::Producer> producer_;
    std::unique_ptr<DeliveryReportCb> delivery_cb_;
    DeliveryCallback callback_;
    std::string errstr_;
    
    std::atomic<size_t> sent_count_{0};
    std::atomic<size_t> failed_count_{0};
};