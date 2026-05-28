#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <algorithm>
#include <array>
#include <type_traits>

using IPpool = std::vector<std::vector<std::string> >;

template<typename... Bytes>
IPpool filter(const IPpool &ip_pool, std::false_type, Bytes... bytes) {
    const std::array<uint8_t, sizeof...(Bytes)> filter_pattern{static_cast<uint8_t>(bytes)...};

    IPpool filtered;
    std::copy_if(ip_pool.cbegin(), ip_pool.cend(), std::back_inserter(filtered), [&filter_pattern](const auto &ip_address) {
        if (ip_address.size() < filter_pattern.size()) {
            return false;
        }

        return std::equal(filter_pattern.cbegin(), filter_pattern.cend(), ip_address.cbegin(),
                          [](const auto pattern_byte, const auto &actual_byte) {
                              return pattern_byte == std::stoi(actual_byte);
                          });
    });

    return filtered;
}

template<typename... Bytes>
IPpool filter(const IPpool &ip_pool, std::true_type, Bytes... bytes) {
    const std::array<int, sizeof...(Bytes)> filter_pattern{static_cast<int>(bytes)...};

    IPpool filtered;
    std::copy_if(ip_pool.cbegin(), ip_pool.cend(), std::back_inserter(filtered), [&filter_pattern](const auto &ip_address) {
        return std::any_of(ip_address.cbegin(), ip_address.cend(), [&filter_pattern](const auto &byte) {
            return std::any_of(filter_pattern.cbegin(), filter_pattern.cend(), [value = std::stoi(byte)](const auto pattern_byte) {
                return pattern_byte == value;
            });
        });
    });

    return filtered;
}