//
// Created by miralius on 29.06.2026.
//

#ifndef TEMPLATES_PRINT_IP_H
#define TEMPLATES_PRINT_IP_H

#include <type_traits>
#include <iostream>

template <typename T>
std::enable_if_t<std::is_integral<T>::value> print_ip(const T &value) {
    const auto unsigned_value = static_cast<std::make_unsigned_t<T>>(value);
    for (std::size_t byte_index = sizeof(T); byte_index > 0; --byte_index) {
        const auto shift = (byte_index - 1) * 8;
        const auto byte = static_cast<unsigned>((unsigned_value >> shift) & 0xff);

        if (byte_index != sizeof(T)) {
            std::cout << '.';
        }
        std::cout << byte;
    }
    std::cout << '\n';
}

template <typename T>
std::enable_if_t<std::is_same<T, std::string>::value> print_ip(const T &value) {
    std::cout << value << '\n';
}

#endif //TEMPLATES_PRINT_IP_H