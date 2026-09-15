#pragma once

#include <librdkafka/rdkafkacpp.h>
#include <string>
#include <memory>
#include <functional>
#include <atomic>
#include <mutex>
#include <unordered_map>
#include <cstdint>

class KafkaProducer
{
public:
    using DeliveryCallback = std::function<void(const std::string &, int, int64_t, int64_t)>; // Четвертый параметр cache_id

    KafkaProducer(const std::string &brokers, const std::string &topic);
    ~KafkaProducer();

    bool init(const std::string &acks = "all", int retries = 3);
    bool send(const std::string &message);
    bool send(const std::string &key, const std::string &message);
    void setDeliveryCallback(DeliveryCallback cb);
    void flush(int timeout_ms = 5000);

    size_t getSentCount() const { return sent_count_.load(); }
    size_t getFailedCount() const { return failed_count_.load(); }

    // Регистрация файла для удаления после доставки
    void registerFileForDeletion(const std::string &key,
                                 const std::string &filename,
                                 int64_t cache_id);
    bool isFilePending(const std::string &filename);
    // Обработка событий доствки (для dr_cb)
    void poll(int timeout_ms = 0);

private:
    class DeliveryReportCb : public RdKafka::DeliveryReportCb
    {
    public:
        DeliveryReportCb(KafkaProducer *producer);
        void dr_cb(RdKafka::Message &msg) override;

    private:
        KafkaProducer *producer_;
    };

    // Структура для хранения связки
    struct PendingFile
    {
        std::string filename;
        int64_t cache_id;
    };

    std::string brokers_;
    std::string topic_;
    std::unique_ptr<RdKafka::Producer> producer_;
    std::unique_ptr<DeliveryReportCb> delivery_cb_;
    DeliveryCallback callback_;
    std::string errstr_;

    std::atomic<size_t> sent_count_{0};
    std::atomic<size_t> failed_count_{0};

    // Маппинг key → {filename, cache_id}
    std::mutex pending_mutex_;
    std::unordered_map<std::string, PendingFile> pending_files_;
};