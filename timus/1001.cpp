//
// Created by miralius on 02.07.2026.
//
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <vector>

int main() {
    std::vector<uint64_t> numbers;

    uint64_t input_number = 0;
    while (std::cin >> input_number) {
        numbers.push_back(input_number);
    }
    std::reverse(numbers.begin(), numbers.end());

    std::cout << std::fixed << std::setprecision(4);

    for (const auto number : numbers) {
        std::cout << std::sqrt(number) << '\n';
    }

    return 0;
}
