#include "producer.hpp"
#include <iostream>

// ============================================================
// DeliveryReportCb - реализация
// ============================================================

KafkaProducer::DeliveryReportCb::DeliveryReportCb(KafkaProducer* producer)
    : producer_(producer) {}

void KafkaProducer::DeliveryReportCb::dr_cb(RdKafka::Message& msg) {
    if (msg.err()) {
        // Ошибка доставки
        producer_->failed_count_.fetch_add(1);
        if (producer_->callback_) {
            producer_->callback_(msg.key() ? *msg.key() : "", msg.err(), msg.offset());
        }
    } else {
        // Успешная доставка
        producer_->sent_count_.fetch_add(1);
        if (producer_->callback_) {
            producer_->callback_(msg.key() ? *msg.key() : "", 0, msg.offset());
        }
    }
}

// ============================================================
// KafkaProducer - реализация
// ============================================================

// Конструктор
KafkaProducer::KafkaProducer(const std::string& brokers, const std::string& topic)
    : brokers_(brokers), topic_(topic), producer_(nullptr), delivery_cb_(nullptr) {}

// Деструктор
KafkaProducer::~KafkaProducer() {
    if (producer_) {
        producer_->flush(5000); // Ждем отправки всех сообщений
        producer_.reset(); // Удаляем producer
    }
}

// ============================================================
// Инициализация producer
// ============================================================
bool KafkaProducer::init(const std::string& acks, int retries) {
    
    // 1. Создаем конфигурацию
    RdKafka::Conf* conf = RdKafka::Conf::create(RdKafka::Conf::CONF_GLOBAL);
    
    // 2. Устанавливаем параметры
    conf->set("bootstrap.servers", brokers_, errstr_);  // Адрес Kafka
    conf->set("acks", acks, errstr_); // Уровень подтверждения
    conf->set("message.send.max.retries", std::to_string(retries), errstr_);
    conf->set("request.timeout.ms", "5000", errstr_);   // Таймаут
    
    // 3. Устанавливаем callback для доставки
    delivery_cb_ = std::make_unique<DeliveryReportCb>(this);
    conf->set("dr_cb", delivery_cb_.get(), errstr_);

    // 4. Создаем producer
    producer_.reset(RdKafka::Producer::create(conf, errstr_));
    delete conf;
    
    if (!producer_) {
        std::cerr << "Failed to create producer: " << errstr_ << std::endl;
        return false;
    }
    
    std::cout << "[Kafka] Producer initialized. Brokers: " << brokers_ 
              << ", Topic: " << topic_ << std::endl;
    return true;
}

// ============================================================
// Отправка сообщения без ключа
// ============================================================
bool KafkaProducer::send(const std::string& message) {
    return send("", message);
}

// ============================================================
// Отправка сообщения с ключом
// ============================================================
bool KafkaProducer::send(const std::string& key, const std::string& message) {
    if (!producer_) {
        std::cerr << "[Kafka] Producer not initialized" << std::endl;
        return false;
    }

    // Вызываем функцию produce библиотеки librdkafka
    RdKafka::ErrorCode err = producer_->produce(
        topic_,                             // Топик
        RdKafka::Topic::PARTITION_UA,       // Авто-выбор партиции
        RdKafka::Producer::RK_MSG_COPY,     // Копировать сообщение
        const_cast<char*>(message.c_str()), // Указатель на данные
        message.size(),                     // Размер данных
        key.empty() ? nullptr : key.c_str(),// Ключ (может быть пустым)
        key.empty() ? 0 : key.size(),       // Размер ключа
        0,                                  // Timestamp (0 = автоматически)
        nullptr                             // Заголовки
    );

    if (err != RdKafka::ERR_NO_ERROR) {
        std::cerr << "[Kafka] Failed to produce: " << RdKafka::err2str(err) << std::endl;
        return false;
    }
    // poll(0) - немедленная отправка (не ждем накопления)
    producer_->poll(0);
    return true;
}

// ============================================================
// Принудительная отправка всех накопленных сообщений
// ============================================================
void KafkaProducer::flush(int timeout_ms) {
    if (producer_) {
        producer_->flush(timeout_ms);
    }
}

// ============================================================
// Установка пользовательского callback
// ============================================================
void KafkaProducer::setDeliveryCallback(DeliveryCallback cb) {
    callback_ = std::move(cb);
}