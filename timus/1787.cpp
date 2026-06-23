//
// Created by miralius on 23.06.2026.
//
#include <iostream>
#include <algorithm>

int main() {
    int k = 0;
    int n = 0;

    std::cin >> k >> n;

    int queueSize = 0;

    for (int i = 0; i < n; ++i) {
        int arrivedCars = 0;
        std::cin >> arrivedCars;

        queueSize += arrivedCars;
        queueSize = std::max(0, queueSize - k);
    }

    std::cout << queueSize << '\n';

    return 0;
}