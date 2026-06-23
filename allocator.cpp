#ifndef ALLOCATOR_H
#define ALLOCATOR_H

#include <map>
#include <iostream>
#include "custom_allocator.h"
#include "custom_container.h"

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
    // 6. создание экземпляра своего контейнера для хранения значений типа int
    custom_list<int> container;
    // 7. заполнение 10 элементами от 0 до 9
    for (int i = 0; i < 10; ++i) container.push_back(i);
    // 8. создание экземпляра своего контейнера для хранения значений типа int с новым аллокатором, ограниченным 10 элементами
    custom_list<int, fixed_allocator<int, 10>> container2;
    // 9. заполнение 10 элементами от 0 до 9
    for (int i = 0; i < 10; ++i) container2.push_back(i);
    // 10. вывод на экран всех значений, хранящихся в контейнере
    for (const auto& value : container2) {
        std::cout << value << std::endl;
    }
    return 0;
}

#endif // ALLOCATOR_H
