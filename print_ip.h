//
// Created by miralius on 29.06.2026.
//

#ifndef TEMPLATES_PRINT_IP_H
#define TEMPLATES_PRINT_IP_H

#include <type_traits>
#include <iostream>
#include <string>
#include <utility>

template <typename...>
using void_t = void;

template <typename, typename = void>
struct is_container : std::false_type {};

template <typename T>
struct is_container<T, void_t<
        typename T::iterator,
        decltype(std::declval<const T &>().begin()),
        decltype(std::declval<const T &>().end())>> : std::true_type {};

template <typename...>
struct are_same : std::true_type {};

template <typename T, typename U, typename... Rest>
struct are_same<T, U, Rest...>
    : std::integral_constant<bool,
                             std::is_same<T, U>::value &&
                                 are_same<T, Rest...>::value> {};

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

template <typename T>
std::enable_if_t<is_container<T>::value && !std::is_same<T, std::decay_t<std::string>>::value>
print_ip(const T &container) {
    auto item = container.begin();

    if (item != container.end()) {
        std::cout << *item++;
    }
    for (; item != container.end(); ++item) {
        std::cout << '.' << *item;
    }
    std::cout << '\n';
}

template <typename T>
void print_tuple(const T &value) {
    std::cout << value;
}

template <typename T, typename... Rest>
void print_tuple(const T &value, const Rest &... rest) {
    std::cout << value << '.';
    print_tuple(rest...);
}

template <typename Tuple, std::size_t... Indices>
void print_tuple(const Tuple &tuple, std::index_sequence<Indices...>) {
    print_tuple(std::get<Indices>(tuple)...);
}

template <typename... Args>
void print_ip(const std::tuple<Args...> &tuple) {
    static_assert(are_same<Args...>::value, "All tuple element types must be the same");

    print_tuple(tuple, std::index_sequence_for<Args...>{});
    std::cout << '\n';
}

#endif //TEMPLATES_PRINT_IP_H
