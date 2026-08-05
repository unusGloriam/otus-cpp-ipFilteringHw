#pragma once

#include <string>
#include <random>
#include <sstream>
#include <iomanip>

namespace utils {

/**
 * @brief Генерирует UUID версии 4 (случайный)
 * @return UUID в формате: 550e8400-e29b-41d4-a716-446655440000
 */
inline std::string generateUUID() {
    std::random_device rd;
    std::mt19937_64 gen(rd());
    std::uniform_int_distribution<int> dist(0, 15);
    std::uniform_int_distribution<int> dist_ver(8, 11);  // Для версии 4
    
    std::stringstream ss;
    ss << std::hex << std::setfill('0');
    
    // Формат: 8-4-4-4-12
    // 8 символов
    for (int i = 0; i < 8; ++i) {
        ss << dist(gen);
    }
    ss << '-';
    
    // 4 символа
    for (int i = 0; i < 4; ++i) {
        ss << dist(gen);
    }
    ss << '-';
    
    // 4 символа (версия 4)
    ss << dist_ver(gen);
    for (int i = 0; i < 3; ++i) {
        ss << dist(gen);
    }
    ss << '-';
    
    // 4 символа (вариант)
    ss << dist(gen);
    for (int i = 0; i < 3; ++i) {
        ss << dist(gen);
    }
    ss << '-';
    
    // 12 символов
    for (int i = 0; i < 12; ++i) {
        ss << dist(gen);
    }
    
    return ss.str();
}

} // namespace utils