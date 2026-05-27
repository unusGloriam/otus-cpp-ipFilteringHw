#include "ip_filter_utils.h"

#include <algorithm>

IPpool filter(const IPpool &ip_pool, uint8_t first_byte) {
    IPpool filtered;
    std::copy_if(ip_pool.cbegin(), ip_pool.cend(), std::back_inserter(filtered), [first_byte](const auto &ip_address) {
        return std::stoi(ip_address.at(0)) == first_byte;
    });
    return filtered;
}

IPpool filter(const IPpool &ip_pool, uint8_t first_byte, uint8_t second_byte) {
    IPpool filtered;
    std::copy_if(ip_pool.cbegin(), ip_pool.cend(), std::back_inserter(filtered),
                 [first_byte, second_byte](const auto &ip_address) {
                     return std::stoi(ip_address.at(0)) == first_byte && std::stoi(ip_address.at(1)) == second_byte;
                 });
    return filtered;
}

IPpool filter_any(const IPpool &ip_pool, uint8_t first_byte) {
    IPpool filtered;
    std::copy_if(ip_pool.cbegin(), ip_pool.cend(), std::back_inserter(filtered), [first_byte](const auto &ip_address) {
        return std::any_of(ip_address.cbegin(), ip_address.cend(), [first_byte](const auto &byte) {
            return std::stoi(byte) == first_byte;
        });
    });
    return filtered;
}
