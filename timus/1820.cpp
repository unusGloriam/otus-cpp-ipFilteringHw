//
// Created by miralius on 02.07.2026.
//
#include <algorithm>
#include <iostream>

int main() {
    int steakCount = 0;
    int panCapacity = 0;

    std::cin >> steakCount >> panCapacity;

    const int sideCount = 2 * steakCount;
    const int minutes = std::max(2, (sideCount + panCapacity - 1) / panCapacity);

    std::cout << minutes << '\n';

    return 0;
}
