# Architecture Design Report
## Kafka-1C Connector

---

## 1. Введение

**Название проекта:** Kafka-1C Connector

**Краткое описание:** Высокопроизводительный микросервис на C++20 для интеграции 1С с Apache Kafka. Обеспечивает двунаправленный обмен заказами между 1С и внешними системами с гарантией доставки и восстановлением после сбоев.

**Версия:** 1.0.0

**Автор:** Ошкин Евгений

---

## 2. Функциональные требования (ФТ)

| ID 	| Требование 			| Описание 								  			| Статус |
|-------|-----------------------|---------------------------------------------------|--------|
| ФТ-01 | Чтение JSON файлов 	| Producer читает JSON файлы из папки input/ 		| 	✅ 	 |
| ФТ-02 | Парсинг JSON 			| Парсинг и валидация структуры заказа 				| 	✅   |
| ФТ-03 | Отправка в Kafka 		| Отправка заказов в топик orders.input 			| 	✅   |
| ФТ-04 | Чтение из Kafka 		| Consumer читает сообщения из топика orders.input 	| 	✅   |
| ФТ-05 | Поиск контрагента 	| Поиск контрагента в PostgreSQL по ИНН/КПП 		| 	✅   |
| ФТ-06 | Создание контрагента 	| Создание нового контрагента, если не найден 		| 	✅   |
| ФТ-07 | Поиск товара 			| Поиск товара по артикулу в PostgreSQL 			| 	✅   |
| ФТ-08 | Создание заказа 		| Создание заказа в PostgreSQL (шапка + товары) 	| 	✅   |
| ФТ-09 | Кэширование 			| Сохранение сообщений в SQLite до обработки 		| 	✅   |
| ФТ-10 | Восстановление 		| Восстановление из кэша после сбоя 				| 	✅   |
| ФТ-11 | Обработка ошибок 		| Отправка ошибок в топик orders.errors 			| 	✅   |
| ФТ-12 | Аудит 				| Запись в регистр сведений _InfoRg60 				| 	✅   |
| ФТ-13 | Режимы работы 		| Producer, Consumer, both 							| 	✅   |

---

## 3. Нефункциональные требования (НФТ)

| ID 	 | Требование 			| Описание 										| Статус |
|--------|----------------------|-----------------------------------------------|--------|
| НФТ-01 | Производительность 	| Lock-free многопоточность через std::atomic 	|   ✅ 	 |
| НФТ-02 | Надежность 			| SQLite кэширование перед отправкой 			|   ✅ 	 |
| НФТ-03 | Отказоустойчивость 	| Автоматическое восстановление из кэша 		|   ✅ 	 |
| НФТ-04 | Масштабируемость 	| Поддержка групп потребителей Kafka 			|   ✅ 	 |
| НФТ-05 | Безопасность 		| acks=all в Kafka Producer 					| 	✅ 	 |
| НФТ-06 | Кодировка 			| Поддержка UTF-8 для кириллицы 				| 	✅ 	 |
| НФТ-07 | Логирование 			| Детальные логи с временными метками 			| 	✅ 	 |
| НФТ-08 | Конфигурация 		| Гибкая настройка через JSON 					| 	✅ 	 |

---

## 4. Архитектура системы

### 4.1 Общая архитектура
┌─────────────────────────────────────────────────────────────────────────────────────┐
│ KAFKA-1C CONNECTOR │
├─────────────────────────────────────────────────────────────────────────────────────┤
│ │
│ ┌─────────────────────────────────┐ ┌─────────────────────────────────────────┐ │
│ │ PRODUCER (Поток 1) │ │ CONSUMER (Поток 2) │ │
│ │ │ │ │ │
│ │ 📁 input/*.json │ │ 📨 Kafka (orders.input) │ │
│ │ ↓ │ │ ↓ │ │
│ │ 🔍 JsonParser::parseOrder() │ │ 🔍 OrderProcessor::processMessage() │ │
│ │ ↓ │ │ ↓ │ │
│ │ ✅ JsonParser::validate() │ │ 🔍 processClient() → PostgreSQL │ │
│ │ ↓ │ │ ↓ │ │
│ │ 💾 MessageCache::save() │ │ 🔍 processProducts() → PostgreSQL │ │
│ │ ↓ │ │ ↓ │ │
│ │ 📤 KafkaProducer::send() │ │ 📝 createOrder() → PostgreSQL │ │
│ │ ↓ │ │ ↓ │ │
│ │ 🗑 fs::remove() │ │ 📤 sendError() → Kafka (ошибки) │ │
│ └─────────────────────────────────┘ └─────────────────────────────────────────┘ │
│ │
│ ┌───────────────────────────────────────────────────────────────────────────────┐ │
│ │ ОБЩИЕ РЕСУРСЫ │ │
│ │ ┌─────────────────┐ ┌─────────────────┐ ┌─────────────────────────────┐ │ │
│ │ │ PostgreSQL │ │ SQLite │ │ Logger │ │ │
│ │ │ (1С таблицы) │ │ Кэш │ │ (Логирование) │ │ │
│ │ └─────────────────┘ └─────────────────┘ └─────────────────────────────┘ │ │
│ └───────────────────────────────────────────────────────────────────────────────┘ │
│ │
└─────────────────────────────────────────────────────────────────────────────────────┘


---

## 5. Диаграмма классов

### 5.1 UML Class Diagram
┌─────────────────────────────────────────────────────────────────────────────────────┐
│ ДИАГРАММА КЛАССОВ │
├─────────────────────────────────────────────────────────────────────────────────────┤
│ │
│ ┌─────────────────────────────────────────┐ │
│ │ AppConfig │ │
│ ├─────────────────────────────────────────┤ │
│ │ - kafka: KafkaConfig │ │
│ │ - database: DatabaseConfig │ │
│ │ - processing: ProcessingConfig │ │
│ │ - cache: CacheConfig │ │
│ │ - groups: vector<GroupConfig> │ │
│ │ - mode: string │ │
│ ├─────────────────────────────────────────┤ │
│ │ + load(filename): AppConfig │ │
│ └─────────────────────────────────────────┘ │
│ │
│ ┌─────────────────────────────────────────┐ │
│ │ KafkaProducer │ │
│ ├─────────────────────────────────────────┤ │
│ │ - brokers_: string │ │
│ │ - topic_: string │ │
│ │ - producer_: unique_ptr<RdKafka::Prod> │ │
│ │ - delivery_cb_: unique_ptr<DeliveryCb> │ │
│ │ - callback_: DeliveryCallback │ │
│ │ - sent_count_: atomic<size_t> │ │
│ │ - failed_count_: atomic<size_t> │ │
│ ├─────────────────────────────────────────┤ │
│ │ + init(acks, retries): bool │ │
│ │ + send(message): bool │ │
│ │ + send(key, message): bool │ │
│ │ + flush(timeout): void │ │
│ │ + setDeliveryCallback(cb): void │ │
│ │ + getSentCount(): size_t │ │
│ │ + getFailedCount(): size_t │ │
│ └─────────────────────────────────────────┘ │
│ │
│ ┌─────────────────────────────────────────┐ │
│ │ KafkaConsumer │ │
│ ├─────────────────────────────────────────┤ │
│ │ - brokers_: string │ │
│ │ - group_id_: string │ │
│ │ - topic_: string │ │
│ │ - consumer_: unique_ptr<RdKafka::Cons> │ │
│ │ - callback_: MessageCallback │ │
│ │ - running_: atomic<bool> │ │
│ │ - consumer_thread_: unique_ptr<thread> │ │
│ ├─────────────────────────────────────────┤ │
│ │ + init(): bool │ │
│ │ + start(): void │ │
│ │ + stop(): void │ │
│ │ + setMessageCallback(cb): void │ │
│ │ + isRunning(): bool │ │
│ │ - consumeLoop(): void │ │
│ └─────────────────────────────────────────┘ │
│ │
│ ┌─────────────────────────────────────────┐ │
│ │ OrderProcessor │ │
│ ├─────────────────────────────────────────┤ │
│ │ - db_: PostgreSQL& │ │
│ │ - cache_: MessageCache& │ │
│ │ - error_producer_: KafkaProducer& │ │
│ ├─────────────────────────────────────────┤ │
│ │ + processMessage(key, value, ts): bool │ │
│ │ + reprocessPendingMessages(): void │ │
│ │ - sendError(type, desc, msg): void │ │
│ │ - getLinuxTime(): long long │ │
│ │ - processClient(order): optional │ │
│ │ - processProducts(order): vector │ │
│ │ - createOrder(order, client, items): │ │
│ │ - logToRegister(order, json): void │ │
│ └─────────────────────────────────────────┘ │
│ │ │ │
│ ┌─────────┴─────────┐ │ │
│ ▼ ▼ ▼ │
│ ┌──────────────────┐ ┌──────────────────┐ ┌──────────────────┐ │
│ │ PostgreSQL │ │ MessageCache │ │ JsonParser │ │
│ ├──────────────────┤ ├──────────────────┤ ├──────────────────┤ │
│ │ - conn_: unique │ │ - db_: sqlite3* │ │ + parseOrder() │ │
│ │ - connected_: boo│ │ - mutex_: mutex │ │ + validate() │ │
│ ├──────────────────┤ ├──────────────────┤ └──────────────────┘ │
│ │ + connect(): boo │ │ + init(): bool │ │
│ │ + execute(): bool│ │ + save(): bool │ ┌──────────────────┐ │
│ │ + query(): resul │ │ + markSent(): bo │ │ OrderData │ │
│ │ + findClient(): │ │ + markError(): bo│ ├──────────────────┤ │
│ │ + createClient() │ │ + getPending(): │ │ - tin: string │ │
│ │ + createOrder() │ │ + removeSent(): │ │ - trrc: string │ │
│ │ + logKafkaMessag │ │ + getPendingCoun│ │ - contractor: st│ │
│ │ - escape(): stri │ │ + getTotalCount(│ │ - date: string │ │
│ │ - generateUUID() │ │ - execute(): boo│ │ - number: string│ │
│ │ - formatDate(): │ └──────────────────┘ │ - goods: vector│ │
│ └──────────────────┘ └──────────────────┘ │
│ │
└─────────────────────────────────────────────────────────────────────────────────────┘


---

## 6. Диаграмма последовательности (Flow Diagram)

### 6.1 Producer Flow
┌─────────────────────────────────────────────────────────────────────────────────────┐
│ FLOW DIAGRAM: PRODUCER (JSON → KAFKA) │
├─────────────────────────────────────────────────────────────────────────────────────┤
│ │
│ [main] [Producer] [JsonParser] [MessageCache] [KafkaProducer] [fs] │
│ │ │ │ │ │ │ │
│ │──run─────▶│ │ │ │ │ │
│ │ │ │ │ │ │ │
│ │ │──loop─────▶│ │ │ │ │
│ │ │ │ │ │ │ │
│ │ │ 1. Чтение файла │ │ │ │
│ │ │──parseOrder(file)────────▶│ │ │ │
│ │ │ │ │ │ │ │
│ │ │ 2. Парсинг JSON │ │ │ │
│ │ │ │──fromJson()──▶│ │ │ │
│ │ │ │◀─OrderData────│ │ │ │
│ │ │ │ │ │ │ │
│ │ │ 3. Валидация │ │ │ │
│ │ │ │──validate()──▶│ │ │ │
│ │ │ │◀─true─────────│ │ │ │
│ │ │ │ │ │ │ │
│ │ │ 4. Сохранение в кэш │ │ │ │
│ │ │──save(file, msg)─────────▶│ │ │ │
│ │ │ │ │──INSERT──────▶│ │ │
│ │ │ │ │◀─OK───────────│ │ │
│ │ │◀─true──────────────────────│ │ │ │
│ │ │ │ │ │ │ │
│ │ │ 5. Отправка в Kafka │ │ │ │
│ │ │──send(key, msg)───────────│──────────────▶│ │ │
│ │ │ │ │ │──produce──▶│ │
│ │ │ │ │ │◀─delivered│ │
│ │ │◀─true───────────────────────────────────────│ │ │
│ │ │ │ │ │ │ │
│ │ │ 6. Удаление файла │ │ │ │
│ │ │──remove(file)──────────────────────────────────────────▶│ │
│ │ │◀─OK──────────────────────────────────────────────────────│ │
│ │ │ │ │ │ │ │
│ │◀─done─────│ │ │ │ │ │
│ │
└─────────────────────────────────────────────────────────────────────────────────────┘


### 6.2 Consumer Flow
┌─────────────────────────────────────────────────────────────────────────────────────┐
│ FLOW DIAGRAM: CONSUMER (KAFKA → POSTGRESQL) │
├─────────────────────────────────────────────────────────────────────────────────────┤
│ │
│ [main] [Consumer] [OrderProcessor] [PostgreSQL] [MessageCache] [Kafka] │
│ │ │ │ │ │ │ │
│ │──run────▶│ │ │ │ │ │
│ │ │ │ │ │ │ │
│ │ │──consume────────────────────────────────────────────────▶│ │
│ │ │◀─message─────────────────────────────────────────────────│ │
│ │ │ │ │ │ │ │
│ │ │ 1. Восстановление из кэша │ │ │ │
│ │ │──reprocessPending()───────▶│ │ │ │
│ │ │ │──getPending()─▶│ │ │ │
│ │ │ │◀─messages──────│ │ │ │
│ │ │ │ │ │ │ │
│ │ │ 2. Обработка сообщения │ │ │ │
│ │ │──callback─────────────────▶│ │ │ │
│ │ │ │ │ │ │ │
│ │ │ 3. Парсинг JSON │ │ │ │
│ │ │ │──fromJson()───▶│ │ │ │
│ │ │ │◀─OrderData─────│ │ │ │
│ │ │ │ │ │ │ │
│ │ │ 4. Сохранение в кэш │ │ │ │
│ │ │ │──save()───────▶│ │ │ │
│ │ │ │ │──INSERT─────▶│ │ │
│ │ │ │ │◀─OK──────────│ │ │
│ │ │ │ │ │ │ │
│ │ │ 5. Поиск контрагента │ │ │ │
│ │ │ │──findClient()─▶│ │ │ │
│ │ │ │ │──SELECT─────▶│ │ │
│ │ │ │ │◀─client─────│ │ │
│ │ │ │◀─client────────│ │ │ │
│ │ │ │ │ │ │ │
│ │ │ 6. Поиск товара │ │ │ │
│ │ │ │──findProduct()▶│ │ │ │
│ │ │ │ │──SELECT─────▶│ │ │
│ │ │ │ │◀─product────│ │ │
│ │ │ │◀─product───────│ │ │ │
│ │ │ │ │ │ │ │
│ │ │ 7. Создание заказа │ │ │ │
│ │ │ │──createOrder()▶│ │ │ │
│ │ │ │ │──INSERT─────▶│ │ │
│ │ │ │ │ (_Document49│ │ │
│ │ │ │ │ _DocumntVT5│ │ │
│ │ │ │ │ _InfoRg60) │ │ │
│ │ │ │ │◀─OK──────────│ │ │
│ │ │ │◀─order_id──────│ │ │ │
│ │ │ │ │ │ │ │
│ │ │ 8. Обновление статуса │ │ │ │
│ │ │ │──markSent()───▶│ │ │ │
│ │ │ │ │──UPDATE─────▶│ │ │
│ │ │ │ │◀─OK──────────│ │ │
│ │ │ │ │ │ │ │
│ │ │ 9. Коммит offset (batch) │ │ │ │
│ │ │──commitAsync()────────────────────────────────────────▶│ │
│ │ │◀─OK─────────────────────────────────────────────────────│ │
│ │ │ │ │ │ │ │
│ │◀─done────│ │ │ │ │ │
│ │
└─────────────────────────────────────────────────────────────────────────────────────┘


### 6.3 Error Flow
┌─────────────────────────────────────────────────────────────────────────────────────┐
│ FLOW DIAGRAM: ERROR HANDLING │
├─────────────────────────────────────────────────────────────────────────────────────┤
│ │
│ [Processor] [sendError] [KafkaProducer] [Kafka] │
│ │ │ │ │ │
│ │ Обнаружена ошибка │ │ │
│ │──sendError(type, desc)──▶│ │ │
│ │ │ │ │ │
│ │ Формирование JSON ошибки│ │ │
│ │ │──generateUUID() │ │
│ │ │──create JSON │ │
│ │ │ { error_type, uuid, │ │
│ │ │ timestamp, description,│ │
│ │ │ original_message } │ │
│ │ │ │ │ │
│ │ Отправка в Kafka │ │ │
│ │ │──send(key, error_msg)───▶│ │
│ │ │ │ │──produce(topic: orders.errors)──▶│
│ │ │ │ │◀─delivered───────────────────────│
│ │ │◀─true─────────────────────│ │
│ │ │ │ │ │
│ │ Логирование ошибки │ │ │
│ │ │──Logger::error() │ │
│ │ │ │ │ │
│ │◀───────────│ │ │ │
│ │
│ Пример ошибки в Kafka: │
│ { │
│ "error_type": "PostgreSQL write error", │
│ "uuid": "550e8400-e29b-41d4-a716-446655440000", │
│ "timestamp": 1712345678901, │
│ "description": "Client not found: TIN=7709876543", │
│ "original_message": "{"TIN":"7709876543",...}" │
│ } │
│ │
└─────────────────────────────────────────────────────────────────────────────────────┘


---

## 7. Компоненты и их ответственность

### 7.1 Таблица компонентов

| Компонент 			| Файл 									| Ответственность 							 |
|-----------------------|---------------------------------------|--------------------------------------------|
| **main** 				| `main.cpp` 							| Точка входа, запуск потоков, инициализация |
| **AppConfig** 		| `config/config.hpp/cpp` 				| Загрузка и хранение конфигурации из JSON 	 |
| **KafkaProducer** 	| `kafka/producer.hpp/cpp` 				| Отправка сообщений в Kafka 				 |
| **KafkaConsumer** 	| `kafka/consumer.hpp/cpp` 				| Получение сообщений из Kafka 				 |
| **OrderProcessor** 	| `processor/order_processor.hpp/cpp` 	| Бизнес-логика обработки заказов 			 |
| **PostgreSQL** 		| `database/postgresql.hpp/cpp` 		| Работа с PostgreSQL (1С таблицы) 			 |
| **MessageCache** 		| `cache/sqlite_cache.hpp/cpp` 			| Кэширование сообщений в SQLite 			 |
| **JsonParser** 		| `parser/json_parser.hpp/cpp` 			| Парсинг и валидация JSON 					 |
| **Logger** 			| `utils/logger.hpp/cpp` 				| Логирование с временными метками 			 |
| **UUID** 				| `utils/uuid.hpp` 						| Генерация UUID 							 |

---

## 8. Потоки данных

### 8.1 Поток Producer
Файл (JSON) → JsonParser::parseOrder() → OrderData → toJson()
↓
MessageCache::save() → SQLite (status='pending')
↓
KafkaProducer::send() → Kafka (topic: orders.input)
↓
fs::remove() → Удаление файла


### 8.2 Поток Consumer
Kafka (topic: orders.input) → KafkaConsumer::consume()
↓
OrderProcessor::processMessage()
↓
JsonParser::fromJson() → OrderData
↓
MessageCache::save() → SQLite (status='pending')
↓
OrderProcessor::logToRegister() → _InfoRg60 (аудит)
↓
OrderProcessor::processClient()
├── PostgreSQL::findClient() → _Reference47
└── PostgreSQL::createClient() → _Reference47 (если не найден)
↓
OrderProcessor::processProducts()
└── PostgreSQL::findProductByArticle() → _Reference48
↓
OrderProcessor::createOrder()
└── PostgreSQL::createOrder()
├── INSERT INTO _Document49 (шапка)
└── INSERT INTO _Document49_VT54 (товары)
↓
MessageCache::markSent() → SQLite (status='sent')
↓
KafkaConsumer::commitAsync() → Коммит offset


### 8.3 Поток ошибок
Ошибка → OrderProcessor::sendError()
↓
Формирование JSON ошибки (uuid, timestamp, description)
↓
KafkaProducer::send() → Kafka (topic: orders.errors)

---

## 9. Таблицы базы данных

### 9.1 Схема PostgreSQL (1С)

```sql
-- Контрагенты
CREATE TABLE _Reference47 (
    _IDRRef UUID PRIMARY KEY,
    _Version BYTEA,
    _Marked BOOLEAN DEFAULT FALSE,
    _Code VARCHAR(20),
    _Description VARCHAR(255),
    _Fld50 VARCHAR(12),   -- ИНН
    _Fld51 VARCHAR(9),    -- КПП
    _PredefinedID UUID
);

-- Номенклатура
CREATE TABLE _Reference48 (
    _IDRRef UUID PRIMARY KEY,
    _Version BYTEA,
    _Marked BOOLEAN DEFAULT FALSE,
    _Code VARCHAR(20),
    _Description VARCHAR(255),
    _Fld52 VARCHAR(50)    -- Артикул
);

-- Заказы (шапка)
CREATE TABLE _Document49 (
    _IDRRef UUID PRIMARY KEY,
    _Version BYTEA,
    _Marked BOOLEAN DEFAULT FALSE,
    _Date_Time TIMESTAMP,
    _Number VARCHAR(50),
    _Posted BOOLEAN DEFAULT TRUE,
    _Fld53RRef UUID REFERENCES _Reference47(_IDRRef)  -- Контрагент
);

-- Товары в заказе (табличная часть)
CREATE TABLE _Document49_VT54 (
    _Document49_IDRRef UUID REFERENCES _Document49(_IDRRef),
    _KeyField UUID,           -- GUID документа (дублирует _Document49_IDRRef)
    _LineNo55 INTEGER,        -- Номер строки
    _Fld56RRef UUID REFERENCES _Reference48(_IDRRef), -- Товар
    _Fld57 NUMERIC(15,3),     -- Количество
    _Fld58 NUMERIC(15,2),     -- Цена
    _Fld59 NUMERIC(15,2)      -- Сумма
);

-- Регистр сведений (аудит)
CREATE TABLE _InfoRg60 (
    _Fld61 BIGINT,            -- Linux timestamp
    _Fld62 VARCHAR(12),       -- ИНН
    _Fld63 VARCHAR(9),        -- КПП
    _Fld64 TEXT               -- JSON заказа
);
```

## 10. Конфигурация
### 10.1 Структура settings.json

{
    "kafka": {
        "bootstrap_servers": "localhost:9092",
        "topics": {
            "input": "orders.input",
            "output": "orders.output",
            "errors": "orders.errors"
        },
        "producer": {
            "acks": "all",
            "retries": 3,
            "batch_size": 100,
            "linger_ms": 5
        },
        "consumer": {
            "group_id": "kafka_1c_consumer",
            "auto_offset_reset": "earliest",
            "enable_auto_commit": true
        }
    },
    "database": {
        "postgresql": {
            "host": "localhost",
            "port": 5432,
            "database": "trade_otus",
            "username": "postgres",
            "password": "Dthibyf7"
        }
    },
    "cache": {
        "path": "cache/messages.db",
        "retention_days": 7,
        "reprocess_delay_seconds": 7,
        "source_prefix": "localhost:9092|orders.input"
    },
    "processing": {
        "max_workers": 4,
        "batch_size": 100,
        "retry_interval_seconds": 60,
        "delete_after_send": true
    },
    "mode": "both",
    "groups": [
        {
            "name": "Main",
            "enabled": true,
            "input_directory": "input",
            "kafka_topic": "orders.input"
        }
    ]
}

## 11. Технические решения
### 11.1 Lock-free многопоточность

// Атомарный счетчик для распределения файлов между потоками
std::atomic<size_t> next_index{0};
while ((idx = next_index.fetch_add(1)) < files.size()) {
    // Обработка файла без блокировок
}

### 11.2 Формат UUID в 1С

// Вход: 550e8400-e29b-41d4-a716-446655440000
// Выход: 0084550e-9be2-d441-a716-446655440000
// Переворот первых 3 групп байт
std::string convertTo1CUUID(const std::string& uuid) {
    // part1: 550e8400 → 0084550e
    // part2: e29b → 9be2
    // part3: 41d4 → d441
    // part4, part5: без изменений
}

### 11.3 Batch commit offset

// Коммит каждые 50 сообщений или каждые 5 секунд
const int BATCH_SIZE = 50;
const int COMMIT_INTERVAL_MS = 5000;

if (messages_since_commit >= BATCH_SIZE || elapsed >= COMMIT_INTERVAL_MS) {
    consumer_->commitAsync(msg);
    messages_since_commit = 0;
}

## 12. Выводы
### 12.1 Достигнутые результаты

✅ Проект полностью реализован на C++20
✅ Producer и Consumer работают в одном приложении
✅ Интеграция с PostgreSQL (1С таблицы)
✅ SQLite кэширование для надежности
✅ Lock-free многопоточность
✅ Автоматическое восстановление после сбоев
✅ Поддержка кириллицы
✅ Документация (Doxygen, README)

### 12.2 Планы на развитие
🔹 Динамические SQL запросы через конфигурацию
🔹 Поддержка RabbitMQ
🔹 GUI для настройки (Qt)
🔹 Метрики и мониторинг (Prometheus)
🔹 Web-интерфейс администратора

## 13. Приложения
### 13.1. Стек технологий

Технология		Версия		Назначение
C++				20			Язык программирования
Apache Kafka	4.2.1		Брокер сообщений
PostgreSQL		17			База данных
SQLite			3			Кэширование
CMake			3.10+		Система сборки
vcpkg			latest		Менеджер зависимостей
librdkafka		2.14.2		Kafka клиент
nlohmann-json	3.12.0		JSON парсер
libpqxx			8.0.2		PostgreSQL клиент

### 13.2. Структура проекта

ProjectC++
├── CMakeLists.txt              # Конфигурация сборки
├── Dockerfile                  # Docker образ
├── Doxyfile                    # Конфигурация Doxygen
├── README.md                   # Документация
├── ARCHITECTURE.md             # Архитектурный отчет
├── config/
│   └── settings.json           # Основная конфигурация
├── input/                      # JSON файлы заказов
├── cache/                      # SQLite база (создается автоматически)
├── src/
│   ├── main.cpp                # Точка входа
│   ├── config/                 # Загрузка конфигурации
│   ├── kafka/                  # Kafka Producer/Consumer
│   ├── parser/                 # Парсинг JSON
│   ├── cache/                  # SQLite кэш
│   ├── database/               # PostgreSQL
│   ├── processor/              # Обработка заказов
│   └── utils/                  # Логирование, UUID
└── build/                      # Сборка (создается)