#include "json_parser.hpp"
#include <fstream>
#include <iostream>
#include <filesystem>
#include "../utils/logger.hpp"

#ifdef _WIN32
#include <windows.h>
#endif

namespace fs = std::filesystem;

// ============================================================
// ★ КОНВЕРТАЦИЯ WINDOWS-1251 → UTF-8 ★
// ============================================================

std::string win1251ToUtf8(const std::string &win1251_str)
{
#ifdef _WIN32
    if (win1251_str.empty())
        return win1251_str;

    int wide_len = MultiByteToWideChar(1251, 0, win1251_str.c_str(), -1, nullptr, 0);
    if (wide_len == 0)
        return win1251_str;

    std::wstring wide_str(wide_len, L'\0');
    MultiByteToWideChar(1251, 0, win1251_str.c_str(), -1, &wide_str[0], wide_len);
    wide_str.pop_back();

    int utf8_len = WideCharToMultiByte(CP_UTF8, 0, wide_str.c_str(), -1, nullptr, 0, nullptr, nullptr);
    if (utf8_len == 0)
        return win1251_str;

    std::string utf8_str(utf8_len, '\0');
    WideCharToMultiByte(CP_UTF8, 0, wide_str.c_str(), -1, &utf8_str[0], utf8_len, nullptr, nullptr);
    utf8_str.pop_back();

    return utf8_str;
#else
    return win1251_str;
#endif
}

// ============================================================
// OrderData::toJson
// ============================================================

std::string OrderData::toJson() const
{
    json j;
    j["TIN"] = tin;
    j["TRRC"] = trrc;
    j["contractor"] = contractor;
    j["date"] = date;
    j["number"] = number;

    json goods_array = json::array();
    for (const auto &item : goods)
    {
        goods_array.push_back({{"SKU", item.sku},
                               {"Quantity", item.quantity},
                               {"Price", item.price},
                               {"Sum", item.sum}});
    }
    j["goods"] = goods_array;

    return j.dump(2);
}

// ============================================================
// OrderData::fromJson (из объекта)
// ============================================================

OrderData OrderData::fromJson(const json &j)
{
    OrderData data;

    data.tin = j.value("TIN", "");
    data.trrc = j.value("TRRC", "");
    data.contractor = j.value("contractor", "");
    data.date = j.value("date", "");
    data.number = j.value("number", "");

    if (j.contains("goods") && j["goods"].is_array())
    {
        for (const auto &item : j["goods"])
        {
            OrderItem order_item;
            order_item.sku = item.value("SKU", "");
            order_item.quantity = item.value("Quantity", 0.0);
            order_item.price = item.value("Price", 0.0);
            order_item.sum = item.value("Sum", 0.0);
            data.goods.push_back(order_item);
        }
    }

    return data;
}

// ============================================================
// OrderData::fromJson (из строки)
// ============================================================

OrderData OrderData::fromJson(const std::string &json_str)
{
    try
    {
        json j = json::parse(json_str); // 4. Парсинг в nlohmann::json
        return fromJson(j);
    }
    catch (const std::exception &e)
    {
        std::cerr << "[JSON] Parse error: " << e.what() << std::endl;
        return OrderData();
    }
}

// ============================================================
// JsonParser::parseOrder - ТОЛЬКО ОДНО ОПРЕДЕЛЕНИЕ
// ============================================================

OrderData JsonParser::parseOrder(const std::string &filename) // 3. Вход в парсинг JSON
{
    std::ifstream file(filename);
    if (!file.is_open())
    {
        std::cerr << "[JSON] Cannot open: " << filename << std::endl;
        return OrderData();
    }

    std::string content((std::istreambuf_iterator<char>(file)),
                        std::istreambuf_iterator<char>());

    // ПРОСТО ПАРСИМ КАК ЕСТЬ - БЕЗ КОНВЕРТАЦИИ
    // PostgreSQL client_encoding = UTF8, а файлы в UTF-8
    return OrderData::fromJson(content);
}

// ============================================================
// JsonParser::parseDirectory
// ============================================================

std::vector<OrderData> JsonParser::parseDirectory(const std::string &directory)
{
    std::vector<OrderData> orders;

    try
    {
        for (const auto &entry : fs::directory_iterator(directory))
        {
            if (entry.path().extension() == ".json")
            {
                OrderData order = parseOrder(entry.path().string());
                if (validate(order))
                {
                    orders.push_back(order);
                    std::cout << "[JSON] Loaded: " << entry.path().filename() << std::endl;
                }
            }
        }
    }
    catch (const std::exception &e)
    {
        std::cerr << "[JSON] Directory error: " << e.what() << std::endl;
    }

    return orders;
}

// ============================================================
// JsonParser::validate
// ============================================================

bool JsonParser::validate(const OrderData &order)
{
    if (order.tin.empty())
    {
        std::cerr << "[JSON] Validation failed: Missing TIN or TRRC" << std::endl;
        return false;
    }
    if (order.goods.empty())
    {
        std::cerr << "[JSON] Validation failed: No goods" << std::endl;
        return false;
    }
    return true;
}