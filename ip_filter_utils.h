#pragma once

#include <cstdint>
#include <string>
#include <vector>

using IPpool = std::vector<std::vector<std::string> >;

IPpool filter(const IPpool &ip_pool, uint8_t first_byte);
IPpool filter(const IPpool &ip_pool, uint8_t first_byte, uint8_t second_byte);
IPpool filter_any(const IPpool &ip_pool, uint8_t first_byte);
