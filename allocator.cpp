#ifndef ALLOCATOR_H
#define ALLOCATOR_H

#include <map>

int factorial(int n) {
    if (n == 0) return 1;
    return n * factorial(n - 1);
}

int main() {
    // 1. создание экземпляра std::map<int, int>
    std::map<int, int> m1;
    for (int i = 0; i < 10; ++i) {
        m1[i] = factorial(i);
    }

    return 0;
}

#endif // ALLOCATOR_H
