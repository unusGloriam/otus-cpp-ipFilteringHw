#pragma once

#include <sqlite3.h>
#include <string>
#include <vector>
#include <tuple>
#include <mutex>
#include <functional> // <-- ДОБАВЛЯЕМ ЭТУ СТРОКУ

// ============================================================
// Класс: MessageCache
// Хранит сообщения в SQLite для надежности
// ============================================================
class MessageCache
{
public:
    MessageCache(const std::string &db_path);
    ~MessageCache();

    // Инициализация БД (создание таблиц)
    bool init();

    // Сохранить сообщение перед отправкой
    bool save(const std::string &filename, const std::string &topic,
              const std::string &message, const std::string &tin = "",
              const std::string &trrc = "", const std::string &source = "");

    void setSourcePrefix(const std::string &prefix) { source_prefix_ = prefix; }
    void setReprocessDelay(int seconds) { reprocess_delay_seconds_ = seconds; }

    // Отметить сообщение как отправленное
    bool markSent(int64_t id);

    // Отметить сообщение как ошибочное
    bool markError(int64_t id, const std::string &error);

    // Получить ожидающие отправки сообщения
    std::vector<std::tuple<int64_t, std::string, std::string, std::string>>
    getPendingMessages(int limit = 100);

    // Удалить отправленные сообщения
    bool removeSent();

    // Очистка старых записей (старше N дней)
    void cleanup(int days = 7);

    // Статистика
    size_t getPendingCount(); // Сколько ожидает отправки
    size_t getTotalCount();   // Всего записей в БД

private:
    std::string db_path_; // Путь к файлу БД
    sqlite3 *db_;         // Указатель на БД
    std::mutex mutex_;    // Мьютекс для потокобезопасности
    std::string source_prefix_;
    int reprocess_delay_seconds_ = 0;

    // Выполнить SQL запрос (без результата)
    bool execute(const std::string &sql);

    // Подготовить и выполнить запрос с параметрами
    bool prepareAndExecute(const std::string &sql,
                           std::function<void(sqlite3_stmt *)> binder);
};