#ifndef ALLOCATOR_H
#define ALLOCATOR_H

#include <map>
#include <iostream>
#include "custom_allocator.h"

int factorial(int n) {
    if (n == 0) return 1;
    return n * factorial(n - 1);
}

int main() {
    // 1. создание экземпляра std::map<int, int>
    std::map<int, int> m1;
    // 2. заполнение 10 элементами, где ключ - это число от 0 до 9, а значение - факториал ключа
    for (int i = 0; i < 10; ++i) {
        m1[i] = factorial(i);
    }
    // 3. создание экземпляра std::map<int, int> с новым аллокатором, ограниченным 10 элементами
    std::map<int, int, std::less<>, fixed_allocator<std::pair<const int, int>, 10>> m2;
    // 4. заполнение 10 элементами, где ключ - это число от 0 до 9, а значение - факториал ключа
    for (int i = 0; i < 10; ++i) {
        m2[i] = factorial(i);
    }
    // 5. вывод на экран всех значений (ключ и значение разделены пробелом), хранящихся в контейнере
    for (const auto& pair : m2) {
        std::cout << "m2[" << pair.first << "] = " << pair.second << std::endl;
    }

    return 0;
}

#endif // ALLOCATOR_H
