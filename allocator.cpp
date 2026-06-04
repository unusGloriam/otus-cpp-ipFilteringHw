#ifndef ALLOCATOR_H
#define ALLOCATOR_H

#include <map>
#include "custom_allocator.h"

int factorial(int n) {
    if (n == 0) return 1;
    return n * factorial(n - 1);
}

int main() {
    // 1. создание экземпляра std::map<int, int>
    // 2. заполнение 10 элементами, где ключ - это число от 0 до 9, а значение - факториал ключа
    std::map<int, int> m1;
    for (int i = 0; i < 10; ++i) {
        m1[i] = factorial(i);
    }
    // 3. создание экземпляра std::map<int, int> с новым аллокатором, ограниченным 10 элементами
    std::map<int, int, std::less<>, fixed_allocator<std::pair<const int, int>, 10>> m2;

    return 0;
}

#endif // ALLOCATOR_H
