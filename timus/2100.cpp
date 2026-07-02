//
// Created by miralius on 02.07.2026.
//
#include <iostream>
#include <string>

int main() {
    int friendCount = 0;
    std::cin >> friendCount;

    int guestCount = 2 + friendCount;

    for (int i = 0; i < friendCount; ++i) {
        std::string response;
        std::cin >> response;

        if (response.find("+one") != std::string::npos) {
            ++guestCount;
        }
    }

    if (guestCount == 13) {
        ++guestCount;
    }

    std::cout << guestCount * 100 << '\n';

    return 0;
}
