#pragma once

#include <string>
#include <iostream>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <mutex>

// ============================================================
// Класс: Logger
// Логирование с временными метками
// ============================================================
class Logger
{
public:
    enum class Level
    {
        DEBUG = 0,
        INFO = 1,
        WARNING = 2,
        ERROR = 3
    };

    static void setLevel(Level level) { current_level_ = level; }

    static void info(const std::string &message)
    {
        log(Level::INFO, message);
    }

    static void warning(const std::string &message)
    {
        log(Level::WARNING, message);
    }

    static void error(const std::string &message)
    {
        log(Level::ERROR, message);
    }

    static void debug(const std::string &message)
    {
        log(Level::DEBUG, message);
    }

private:
    static Level current_level_;
    static std::mutex mutex_;

    static void log(Level level, const std::string &message)
    {
        if (level < current_level_)
            return; // Если уровень ниже текущего - пропускаем

        std::lock_guard<std::mutex> lock(mutex_); // Защита от перемешивания логов

        std::string prefix;
        switch (level)
        {
        case Level::INFO:
            prefix = "[INFO] ";
            break;
        case Level::WARNING:
            prefix = "[WARN] ";
            break;
        case Level::ERROR:
            prefix = "[ERROR] ";
            break;
        case Level::DEBUG:
            prefix = "[DEBUG] ";
            break;
        }

        std::cout << prefix << getTimestamp() << " " << message << std::endl;
    }

    static std::string getTimestamp()
    {
        auto now = std::chrono::system_clock::now();
        auto time_t = std::chrono::system_clock::to_time_t(now);
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                      now.time_since_epoch()) %
                  1000;

        std::tm tm;
#ifdef _WIN32
        localtime_s(&tm, &time_t);
#else
        localtime_r(&time_t, &tm);
#endif

        std::ostringstream oss;
        oss << std::put_time(&tm, "%Y-%m-%d %H:%M:%S")
            << "." << std::setfill('0') << std::setw(3) << ms.count();
        return oss.str();
    }
};

// Инициализация статических членов
inline Logger::Level Logger::current_level_ = Logger::Level::INFO;
inline std::mutex Logger::mutex_;