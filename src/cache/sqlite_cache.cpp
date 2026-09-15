#include "sqlite_cache.hpp"
#include <iostream>

// ============================================================
// Конструктор
// ============================================================
MessageCache::MessageCache(const std::string &db_path)
    : db_path_(db_path), db_(nullptr) {}

// ============================================================
// Деструктор
// ============================================================
MessageCache::~MessageCache()
{
    if (db_)
    {
        sqlite3_close(db_); // Закрываем соединение с БД
    }
}

// ============================================================
// Инициализация
// 1. Открываем БД (создается автоматически)
// 2. Включаем WAL режим для производительности
// 3. Создаем таблицу messages
// ============================================================
bool MessageCache::init()
{
    std::lock_guard<std::mutex> lock(mutex_);

    if (sqlite3_open(db_path_.c_str(), &db_) != SQLITE_OK)
    {
        std::cerr << "[Cache] Failed to open: " << sqlite3_errmsg(db_) << std::endl;
        return false;
    }

    execute("PRAGMA journal_mode=WAL;");
    execute("PRAGMA synchronous=NORMAL;");

    // ДОБАВЛЯЕМ ПОЛЕ source
    const char *sql = R"(
        CREATE TABLE IF NOT EXISTS messages (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            filename TEXT NOT NULL,
            tin TEXT,
            trrc TEXT,
            topic TEXT NOT NULL,
            message TEXT NOT NULL,
            status TEXT DEFAULT 'pending',
            source TEXT NOT NULL,                    -- ← УНИКАЛЬНЫЙ ИДЕНТИФИКАТОР
            created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
            sent_at TIMESTAMP,
            error TEXT
        );
        CREATE INDEX IF NOT EXISTS idx_status ON messages(status);
        CREATE INDEX IF NOT EXISTS idx_created ON messages(created_at);
        CREATE INDEX IF NOT EXISTS idx_source ON messages(source);
    )";

    if (!execute(sql))
    {
        return false;
    }

    std::cout << "[Cache] Initialized: " << db_path_ << std::endl;
    return true;
}

// ============================================================
// Сохранение сообщения в кэш
// ============================================================
int64_t MessageCache::save(const std::string &filename, const std::string &topic,
                           const std::string &message, const std::string &tin,
                           const std::string &trrc, const std::string &source)
{
    std::lock_guard<std::mutex> lock(mutex_);

    std::string sql = R"(
        INSERT INTO messages (filename, tin, trrc, topic, message, status, source)
        VALUES (?, ?, ?, ?, ?, 'pending', ?)
    )";

    sqlite3_stmt *stmt;
    if (sqlite3_prepare_v2(db_, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK)
    {
        std::cerr << "[Cache] Prepare failed: " << sqlite3_errmsg(db_) << std::endl;
        return false;
    }

    sqlite3_bind_text(stmt, 1, filename.c_str(), -1, SQLITE_TRANSIENT); // 6. Привязка параметров к INSERT
    sqlite3_bind_text(stmt, 2, tin.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 3, trrc.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 4, topic.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 5, message.c_str(), -1, SQLITE_TRANSIENT);

    // ★ source - уникальный идентификатор ★
    std::string full_source = source;
    if (!source_prefix_.empty() && !source.empty())
    {
        full_source = source_prefix_ + "|" + source;
    }
    sqlite3_bind_text(stmt, 6, full_source.c_str(), -1, SQLITE_TRANSIENT);

    int rc = sqlite3_step(stmt); // 7. Выполнение INSERT
    sqlite3_finalize(stmt);

    if (rc != SQLITE_DONE)
    {
        std::cerr << "[Cache] Insert failed: " << sqlite3_errmsg(db_) << std::endl;
        return false;
    }

    return sqlite3_last_insert_rowid(db_);
}

// ============================================================
// Отметка сообщения как отправленного
// ============================================================
bool MessageCache::markSent(int64_t id)
{
    std::lock_guard<std::mutex> lock(mutex_);

    // 27. Выполнение UPDATE изменение статуса
    std::string sql = R"(
        UPDATE messages 
        SET status = 'sent', sent_at = CURRENT_TIMESTAMP 
        WHERE id = ?
    )"; // Формирование SQL

    sqlite3_stmt *stmt;
    if (sqlite3_prepare_v2(db_, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) // Выполнение запроса
    {
        return false;
    }

    sqlite3_bind_int64(stmt, 1, id);
    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    return rc == SQLITE_DONE;
}

bool MessageCache::markError(int64_t id, const std::string &error)
{
    std::lock_guard<std::mutex> lock(mutex_);

    std::string sql = R"(
        UPDATE messages 
        SET status = 'error', error = ? 
        WHERE id = ?
    )";

    sqlite3_stmt *stmt;
    if (sqlite3_prepare_v2(db_, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK)
    {
        return false;
    }

    sqlite3_bind_text(stmt, 1, error.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int64(stmt, 2, id);
    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    return rc == SQLITE_DONE;
}

// ============================================================
// Получение ожидающих отправки сообщений
// ============================================================
std::vector<std::tuple<int64_t, std::string, std::string, std::string>>
MessageCache::getPendingMessages(int limit)
{
    std::lock_guard<std::mutex> lock(mutex_);

    std::vector<std::tuple<int64_t, std::string, std::string, std::string>> result;

    // ИСПОЛЬЗУЕМ source_prefix ДЛЯ ФИЛЬТРАЦИИ
    std::string sql = R"(
        SELECT id, topic, message, filename 
        FROM messages 
        WHERE status = 'pending' 
        AND source LIKE ? || '%'
        AND datetime(created_at) <= datetime('now', '-' || ? || ' seconds')
        ORDER BY created_at 
        LIMIT ?
    )";

    sqlite3_stmt *stmt;
    if (sqlite3_prepare_v2(db_, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK)
    {
        std::cerr << "[Cache] Select failed: " << sqlite3_errmsg(db_) << std::endl;
        return result;
    }

    // sqlite3_bind_text(stmt, 1, source_prefix_.c_str(), -1, SQLITE_TRANSIENT);
    std::string producer_prefix = source_prefix_ + "|producer";
    sqlite3_bind_text(stmt, 1, producer_prefix.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 2, reprocess_delay_seconds_);
    sqlite3_bind_int(stmt, 3, limit);

    while (sqlite3_step(stmt) == SQLITE_ROW)
    {
        int64_t id = sqlite3_column_int64(stmt, 0);
        std::string topic = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 1));
        std::string message = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 2));
        std::string filename = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 3));
        result.emplace_back(id, topic, message, filename);
    }

    sqlite3_finalize(stmt);
    return result;
}

bool MessageCache::removeSent()
{
    std::lock_guard<std::mutex> lock(mutex_);
    return execute("DELETE FROM messages WHERE status = 'sent'");
}

void MessageCache::cleanup(int days)
{
    std::lock_guard<std::mutex> lock(mutex_);
    std::string sql = "DELETE FROM messages WHERE created_at < datetime('now', '-' || ? || ' days')";

    sqlite3_stmt *stmt;
    if (sqlite3_prepare_v2(db_, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK)
    {
        return;
    }

    sqlite3_bind_int(stmt, 1, days);
    sqlite3_step(stmt);
    sqlite3_finalize(stmt);
}

// ============================================================
// Статистика: сколько ожидающих отправки
// ============================================================
size_t MessageCache::getPendingCount()
{
    std::lock_guard<std::mutex> lock(mutex_);

    sqlite3_stmt *stmt;
    if (sqlite3_prepare_v2(db_, "SELECT COUNT(*) FROM messages WHERE status = 'pending'",
                           -1, &stmt, nullptr) != SQLITE_OK)
    {
        return 0;
    }

    sqlite3_step(stmt);
    size_t count = sqlite3_column_int64(stmt, 0);
    sqlite3_finalize(stmt);
    return count;
}

// ============================================================
// Статистика: всего записей
// ============================================================
size_t MessageCache::getTotalCount()
{
    std::lock_guard<std::mutex> lock(mutex_);

    sqlite3_stmt *stmt;
    if (sqlite3_prepare_v2(db_, "SELECT COUNT(*) FROM messages", -1, &stmt, nullptr) != SQLITE_OK)
    {
        return 0;
    }

    sqlite3_step(stmt);
    size_t count = sqlite3_column_int64(stmt, 0);
    sqlite3_finalize(stmt);
    return count;
}

// ============================================================
// Вспомогательная функция: выполнить SQL запрос
// ============================================================
bool MessageCache::execute(const std::string &sql)
{
    char *errmsg = nullptr;
    int rc = sqlite3_exec(db_, sql.c_str(), nullptr, nullptr, &errmsg);
    if (rc != SQLITE_OK)
    {
        std::cerr << "[Cache] SQL error: " << errmsg << std::endl;
        sqlite3_free(errmsg);
        return false;
    }
    return true;
}

bool MessageCache::prepareAndExecute(const std::string &sql,
                                     std::function<void(sqlite3_stmt *)> binder)
{
    // Этот метод пока не используется, но оставлен для будущего расширения
    return true;
}