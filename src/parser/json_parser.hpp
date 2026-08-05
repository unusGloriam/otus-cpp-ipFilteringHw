#pragma once

#include <string>
#include <vector>
#include <nlohmann/json.hpp>

// ============================================================
// Структура: OrderItem
// Один товар в заказе
// ============================================================
using json = nlohmann::json;

struct OrderItem {
    std::string sku;       // Артикул товара
    double quantity;       // Количество
    double price;          // Цена за единицу
    double sum;            // Сумма (quantity * price)
};

// ============================================================
// Структура: OrderData
// Полный заказ (контрагент + товары)
// ============================================================
struct OrderData {
    std::string tin;           // ИНН контрагента
    std::string trrc;          // КПП контрагента
    std::string contractor;    // Наименование контрагента
    std::string date;          // Дата документа
    std::string number;        // Номер документа
    std::vector<OrderItem> goods; // Массив товаров
    
    // Преобразование в JSON строку
    std::string toJson() const;
    // Создание из JSON строки
    static OrderData fromJson(const std::string& json_str);
    // Создание из JSON объекта
    static OrderData fromJson(const json& j);
};

// ============================================================
// Класс: JsonParser
// Парсинг JSON файлов
// ============================================================
class JsonParser {
public:
    // Парсинг одного файла
    static OrderData parseOrder(const std::string& filename);
    // Парсинг из строки
    static OrderData parseOrderFromString(const std::string& json_str);
    // Парсинг всех файлов в папке
    static std::vector<OrderData> parseDirectory(const std::string& directory);
    // Валидация заказа (проверка обязательных полей)
    static bool validate(const OrderData& order);
};