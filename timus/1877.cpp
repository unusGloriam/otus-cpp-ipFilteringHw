//
// Created by miralius on 23.06.2026.
//
#include <iostream>

int main() {
    int firstCode = 0;
    int secondCode = 0;

    std::cin >> firstCode >> secondCode;

    if (firstCode % 2 == 0 || secondCode % 2 == 1) {
        std::cout << "yes\n";
    } else {
        std::cout << "no\n";
    }

    return 0;
}