# Kafka-1C Connector

**Высокопроизводительный микросервис для интеграции 1С с Apache Kafka на C++**

[![C++](https://img.shields.io/badge/C++-20-blue.svg)](https://isocpp.org/)
[![Kafka](https://img.shields.io/badge/Kafka-4.2.1-green.svg)](https://kafka.apache.org/)
[![PostgreSQL](https://img.shields.io/badge/PostgreSQL-16-blue.svg)](https://www.postgresql.org/)
[![License](https://img.shields.io/badge/license-MIT-green.svg)](LICENSE)

---

## 📋 Описание

**Kafka-1C Connector** — это production-ready микросервис на C++20 для высокоскоростной интеграции 1С с Apache Kafka. Обеспечивает надежную передачу заказов между 1С и внешними системами с гарантией доставки и восстановлением после сбоев.

### Ключевые возможности

| Возможность | Описание |
|-------------|----------|
| **🔁 Двунаправленная интеграция** | Producer (1С → Kafka) и Consumer (Kafka → 1С) в одном приложении |
| **💾 Надежность** | SQLite кэширование всех сообщений с восстановлением после сбоев |
| **🚀 Производительность** | Lock-free многопоточная обработка с атомарными операциями |
| **🔄 Отказоустойчивость** | Автоматическое восстановление неотправленных сообщений при перезапуске |
| **📊 Мониторинг** | Детальное логирование и статистика по всем операциям |
| **⚙️ Гибкость** | JSON-конфигурация всех параметров (топики, потоки, папки) |
| **🐳 Docker** | Готовый Dockerfile для развертывания в контейнере |

---

## 🏗 Архитектура
┌─────────────────────────────────────────────────────────────────────────────┐
│ Kafka-1C Connector │
├─────────────────────────────────────────────────────────────────────────────┤
│ │
│ ┌─────────────────────────────┐ ┌──────────────────────────────────┐ │
│ │ PRODUCER (Поток 1) │ │ CONSUMER (Поток 2) │ │
│ │ │ │ │ │
│ │ 📁 input/*.json │ │ 📨 Kafka (orders.input) │ │
│ │ ↓ │ │ ↓ │ │
│ │ 🔍 Парсинг JSON │ │ 🔍 Парсинг JSON │ │
│ │ ↓ │ │ ↓ │ │
│ │ 💾 SQLite кэш (pending) │ │ 🔍 Поиск контрагента │ │
│ │ ↓ │ │ ↓ │ │
│ │ 📤 Kafka (orders.input) │ │ 🔍 Поиск товаров │ │
│ │ ↓ │ │ ↓ │ │
│ │ ✅ Обновление статуса │ │ 📝 Создание заказа │ │
│ │ 🗑 Удаление файла │ │ ↓ │ │
│ └─────────────────────────────┘ │ 💾 SQLite кэш (sent/error) │ │
│ │ ↓ │ │
│ │ 📤 Kafka (orders.errors) │ │
│ └──────────────────────────────────┘ │
│ │
│ ┌─────────────────────────────────────────────────────────────────────┐ │
│ │ ОБЩИЕ РЕСУРСЫ │ │
│ │ • SQLite кэш (cache/messages.db) │ │
│ │ • PostgreSQL (trade_otus) │ │
│ │ • Конфигурация (config/settings.json) │ │
│ │ • Логгер с временными метками │ │
│ └─────────────────────────────────────────────────────────────────────┘ │
│ │
└─────────────────────────────────────────────────────────────────────────────┘


---

## 🚀 Быстрый старт

### Требования

| Компонент | Версия | Описание |
|-----------|--------|----------|
| **C++** | 20 | Стандарт языка |
| **CMake** | 3.10+ | Система сборки |
| **Apache Kafka** | 4.2.1+ | Брокер сообщений |
| **PostgreSQL** | 16+ | База данных |
| **vcpkg** | latest | Менеджер пакетов |

### Зависимости (vcpkg)

```powershell
vcpkg install librdkafka:x64-windows
vcpkg install nlohmann-json:x64-windows
vcpkg install sqlite3:x64-windows
vcpkg install libpqxx:x64-windows

Сборка
Windows (Visual Studio)
git clone <repository>
cd ProjectC++
mkdir build && cd build
cmake .. -DCMAKE_TOOLCHAIN_FILE="C:/vcpkg/scripts/buildsystems/vcpkg.cmake" -DCMAKE_BUILD_TYPE=Release
cmake --build . --config Release

Linux / macOS
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)

Запуск
1. Запустите Kafka
# Linux
cd /opt/kafka
./bin/kafka-server-start.sh config/server.properties

# Windows
cd D:\kafka
.\bin\windows\kafka-server-start.bat .\config\server.properties

2. Создайте топики
# Linux
/opt/kafka/bin/kafka-topics.sh --create \
  --topic orders.input \
  --bootstrap-server localhost:9092 \
  --partitions 3 \
  --replication-factor 1

/opt/kafka/bin/kafka-topics.sh --create \
  --topic orders.errors \
  --bootstrap-server localhost:9092 \
  --partitions 1 \
  --replication-factor 1

# Windows
.\bin\windows\kafka-topics.bat --create --topic orders.input --bootstrap-server localhost:9092 --partitions 3 --replication-factor 1

3. Запустите микросервис
cd build/Release
./kafka_order_sender.exe           # Режим both (Producer + Consumer)
./kafka_order_sender.exe producer  # Только Producer
./kafka_order_sender.exe consumer  # Только Consumer

⚙️ Конфигурация
config/settings.json
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
            "password": "your_password"
        }
    },
    "cache": {
        "path": "cache/messages.db"
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

								Параметры конфигурации
Параметр							Описание							Значение по умолчанию
kafka.bootstrap_servers			Адрес Kafka брокера							localhost:9092
kafka.producer.acks				Уровень подтверждения (0, 1, all)			all
kafka.producer.retries			Количество попыток отправки					3
kafka.consumer.group_id			ID группы потребителей						kafka_1c_consumer
database.postgresql.host		Хост PostgreSQL								localhost
database.postgresql.port		Порт PostgreSQL								5432
processing.max_workers			Максимальное количество потоков				4
processing.delete_after_send	Удалять файлы после отправки				true
mode							Режим работы (producer, consumer, both)		both

📁 Формат заказа (JSON)
Пример файла input/order_1.json
{
    "TIN": "7701234567",
    "TRRC": "770101001",
    "contractor": "ООО Тестовая Компания",
    "date": "2026-08-02T10:00:00",
    "number": "ORD-001",
    "goods": [
        {
            "SKU": "PROD-001",
            "Quantity": 10,
            "Price": 1500.50,
            "Sum": 15005.00
        },
        {
            "SKU": "PROD-002",
            "Quantity": 5,
            "Price": 2500.00,
            "Sum": 12500.00
        }
    ]
}

						Поля заказа
Поле					Тип				Обязательное			Описание
TIN						string				✅				ИНН контрагента (12 цифр)
TRRC					string				✅				КПП контрагента (9 цифр)
contractor				string				❌				Наименование контрагента
date					string				❌				Дата документа (ISO 8601)
number					string				❌				Номер документа
goods					array				✅				Массив товаров
SKU						string				✅				Артикул товара
Quantity				number				✅				Количество
Price					number				✅				Цена за единицу
Sum						number				✅				Сумма


🔄 Работа с PostgreSQL
Схема базы данных (1С)
-- Контрагенты
CREATE TABLE _Reference47 (
    _IDRRef UUID PRIMARY KEY,
    _Code VARCHAR(20),
    _Description VARCHAR(255),
    _Fld50 VARCHAR(12),   -- ИНН
    _Fld51 VARCHAR(9),    -- КПП
    _Marked BOOLEAN DEFAULT FALSE
);

-- Номенклатура
CREATE TABLE _Reference48 (
    _IDRRef UUID PRIMARY KEY,
    _Code VARCHAR(20),
    _Description VARCHAR(255),
    _Fld52 VARCHAR(50),   -- Артикул
    _Marked BOOLEAN DEFAULT FALSE
);

-- Заказы
CREATE TABLE _Document49 (
    _IDRRef UUID PRIMARY KEY,
    _Date_Time TIMESTAMP,
    _Number VARCHAR(50),
    _Posted BOOLEAN DEFAULT TRUE,
    _Marked BOOLEAN DEFAULT FALSE,
    _Fld53RRef UUID REFERENCES _Reference47(_IDRRef)
);

-- Товары в заказе
CREATE TABLE _Document49_VT54 (
    _Document49_IDRRef UUID REFERENCES _Document49(_IDRRef),
    _Fld56RRef UUID REFERENCES _Reference48(_IDRRef),
    _Fld57 NUMERIC(15, 3),
    _Fld58 NUMERIC(15, 2),
    _Fld59 NUMERIC(15, 2)
);

-- Регистр сведений (логи)
CREATE TABLE _InfoRg60 (
    _Fld61 BIGINT,        -- Linux timestamp
    _Fld62 VARCHAR(12),   -- ИНН
    _Fld63 VARCHAR(9),    -- КПП
    _Fld64 TEXT           -- JSON данные
);

Логика обработки заказа
1. Поиск контрагента по TIN и TRRC

2. Если не найден — создание нового контрагента

3. Поиск товаров по артикулам (SKU)

4. Создание заказа в документе и таблице товаров

5. Запись в регистр сведений (аудит)

📊 Мониторинг
Просмотр сообщений в Kafka

# Просмотр всех сообщений
kafka-console-consumer --topic orders.input --bootstrap-server localhost:9092 --from-beginning

# Просмотр ошибок
kafka-console-consumer --topic orders.errors --bootstrap-server localhost:9092 --from-beginning

# Просмотр с фильтром по ключу
kafka-console-consumer --topic orders.input --bootstrap-server localhost:9092 --property print.key=true --property key.separator=:

Просмотр кэша SQLite
# Установить sqlite3
sqlite3 cache/messages.db

# Просмотр всех сообщений
SELECT * FROM messages;

# Просмотр только ожидающих
SELECT * FROM messages WHERE status = 'pending';

# Статистика по статусам
SELECT status, COUNT(*) FROM messages GROUP BY status;

Логирование
Программа выводит логи в консоль с временными метками:
[INFO] 2026-08-02 14:30:15.123 === Kafka-1C Connector (Producer + Consumer) ===
[INFO] 2026-08-02 14:30:15.124 Version: 1.0.0
[INFO] 2026-08-02 14:30:15.125 Config loaded successfully
[INFO] 2026-08-02 14:30:15.126 Cache initialized: cache/messages.db
[INFO] 2026-08-02 14:30:15.127 PostgreSQL connected: trade_otus
[Kafka] Producer initialized. Brokers: localhost:9092, Topic: orders.input
[INFO] 2026-08-02 14:30:15.128 Mode: both
[INFO] 2026-08-02 14:30:15.129 Starting Producer thread...
[INFO] 2026-08-02 14:30:17.130 Starting Consumer thread...
[INFO] 2026-08-02 14:30:17.131 Found 1 files in input
[JSON] Loaded: order_1.json
[INFO] 2026-08-02 14:30:17.132 [Producer] Sent: 7701234567 (1)
[DEBUG] 2026-08-02 14:30:17.133 [Producer] Delivered: 7701234567 (offset: 0)
[INFO] 2026-08-02 14:30:17.134 Order processed successfully: ORD-001

🐳 Docker
Сборка образа
docker build -t kafka-1c-connector .

Запуск контейнера
docker run -p 9092:9092 -p 5432:5432 kafka-1c-connector

📁 Структура проекта
ProjectC++
├── CMakeLists.txt              # Конфигурация сборки
├── Dockerfile                  # Docker образ
├── Doxyfile                    # Конфигурация Doxygen
├── README.md                   # Документация
├── .gitignore                  # Игнорируемые файлы
│
├── config/
│   └── settings.json           # Основная конфигурация
│
├── input/                      # Папка для JSON файлов
│   └── order_1.json            # Пример заказа
│
├── cache/                      # SQLite база (создается автоматически)
│   └── messages.db             # Кэш сообщений
│
└── src/
    ├── main.cpp                # Точка входа
    ├── config/
    │   ├── config.hpp
    │   └── config.cpp          # Загрузка конфигурации
    ├── kafka/
    │   ├── producer.hpp
    │   ├── producer.cpp        # Kafka Producer
    │   ├── consumer.hpp
    │   └── consumer.cpp        # Kafka Consumer
    ├── parser/
    │   ├── json_parser.hpp
    │   └── json_parser.cpp     # Парсинг JSON
    ├── cache/
    │   ├── sqlite_cache.hpp
    │   └── sqlite_cache.cpp    # SQLite кэш
    ├── database/
    │   ├── postgresql.hpp
    │   └── postgresql.cpp      # PostgreSQL
    ├── processor/
    │   ├── order_processor.hpp
    │   └── order_processor.cpp # Обработка заказов
    └── utils/
        ├── logger.hpp
        ├── logger.cpp          # Логирование
        └── uuid.hpp            # Генерация UUID
		
🔧 Отладка
Включение DEBUG логов
В main.cpp добавьте:
Logger::setLevel(Logger::Level::DEBUG);

Брейкпоинты для отладки
Файл									Строка								Что проверять
main.cpp					AppConfig config = AppConfig::load(...)		Загрузка конфига
producer.cpp				producer_->produce(...)						Отправка в Kafka
consumer.cpp				consumer_->consume(...)						Получение из Kafka
order_processor.cpp			processMessage(...)							Обработка заказа
postgresql.cpp				txn.exec(...)								Выполнение SQL

📈 План развития
✅ Сделано (MVP)
☑ Kafka Producer с многопоточностью
☑ Kafka Consumer с автоматическим коммитом
☑ SQLite кэширование и восстановление
☑ Интеграция с PostgreSQL (1С схема)
☑ JSON-конфигурация
☑ Логирование
☑ Docker-контейнеризация
🚧 В планах (Релиз 2.0)
□ Динамические SQL запросы через конфигурацию
□ Поддержка RabbitMQ
□ GUI для настройки (Qt)
□ Метрики и мониторинг (Prometheus)
□ Кластеризация и балансировка
□ Web-интерфейс администратора

📄 Лицензия
MIT License. См. файл LICENSE.

📧 Контакты
Автор: Evgeniy Oshkin

Email: oshkines@mail.ru

GitHub: oshkines/otus-cpp-prof

⭐ Поддержка
Если проект оказался полезным, поставьте звезду на GitHub! Это поможет другим разработчикам найти его.

Сделано с ❤️ для бешеной скорости интеграции 1С и Kafka

© 2026 Evgeniy Oshkin. Все права защищены.
