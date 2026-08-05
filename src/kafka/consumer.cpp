#include "consumer.hpp"
#include "../utils/logger.hpp"
#include <iostream>

KafkaConsumer::KafkaConsumer(const std::string& brokers, 
                             const std::string& group_id,
                             const std::string& topic)
    : brokers_(brokers), group_id_(group_id), topic_(topic), 
      consumer_(nullptr), running_(false) {}

KafkaConsumer::~KafkaConsumer() {
    stop();
}

bool KafkaConsumer::init() {
    RdKafka::Conf* conf = RdKafka::Conf::create(RdKafka::Conf::CONF_GLOBAL);
    
    conf->set("bootstrap.servers", brokers_, errstr_);
    conf->set("group.id", group_id_, errstr_);
    conf->set("enable.auto.commit", "true", errstr_);  		// ← ВКЛЮЧАЕМ АВТО-КОММИТ
	conf->set("auto.commit.interval.ms", "1000", errstr_);  // ← КАЖДУЮ СЕКУНДУ
    conf->set("auto.offset.reset", "earliest", errstr_);
    conf->set("session.timeout.ms", "6000", errstr_);
    
    consumer_.reset(RdKafka::KafkaConsumer::create(conf, errstr_));
    delete conf;
    
    if (!consumer_) {
        Logger::error("Failed to create consumer: " + errstr_);
        return false;
    }
    
    // Подписываемся на топик
    std::vector<std::string> topics = {topic_};
    RdKafka::ErrorCode err = consumer_->subscribe(topics);
    if (err != RdKafka::ERR_NO_ERROR) {
        Logger::error("Failed to subscribe: " + RdKafka::err2str(err));
        return false;
    }
    
    Logger::info("Kafka consumer initialized. Brokers: " + brokers_ + ", Topic: " + topic_);
    return true;
}

void KafkaConsumer::start() {
    if (running_.load()) return;
    
    running_.store(true);
    consumer_thread_ = std::make_unique<std::thread>(&KafkaConsumer::consumeLoop, this);
    Logger::info("Kafka consumer started");
}

void KafkaConsumer::stop() {
    if (!running_.load()) return;
    
    running_.store(false);
    if (consumer_thread_ && consumer_thread_->joinable()) {
        consumer_thread_->join();
    }
    consumer_thread_.reset();
    
    if (consumer_) {
        consumer_->close();
    }
    
    Logger::info("Kafka consumer stopped");
}

void KafkaConsumer::setMessageCallback(MessageCallback cb) {
    callback_ = std::move(cb);
}

void KafkaConsumer::consumeLoop() {
    Logger::info("Consumer loop started for topic: " + topic_);
    
    while (running_.load()) {
        RdKafka::Message* msg = consumer_->consume(1000); // 1 секунда таймаут
        
        if (!msg) continue;
        
        if (msg->err() == RdKafka::ERR_NO_ERROR) {
            // Получаем сообщение
            std::string key = msg->key() ? *msg->key() : "";
            std::string value(static_cast<const char*>(msg->payload()), msg->len());
            int64_t timestamp = msg->timestamp().timestamp;
            
            Logger::debug("Received message: " + key);
            
            if (callback_) {
                callback_(key, value, timestamp);
            }
             // АСИНХРОННЫЙ КОММИТ (не блокирует)
            consumer_->commitAsync(msg);           
        } else if (msg->err() == RdKafka::ERR__PARTITION_EOF) {
            // Конец партиции — нормально
        } else {
            Logger::error("Consumer error: " + msg->errstr());
        }
        
        delete msg;
    }
    
    Logger::info("Consumer loop stopped");
}