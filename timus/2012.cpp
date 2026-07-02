//
// Created by miralius on 02.07.2026.
//
#include <iostream>

int main() {
    int solvedInFirstHour = 0;
    std::cin >> solvedInFirstHour;

    const int remainingProblems = 12 - solvedInFirstHour;

    if (constexpr int remainingMinutes = 4 * 60;
        remainingProblems * 45 <= remainingMinutes) {
        std::cout << "YES\n";
    } else {
        std::cout << "NO\n";
    }

    return 0;
}
