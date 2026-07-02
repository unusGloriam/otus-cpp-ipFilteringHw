//
// Created by miralius on 02.07.2026.
//
#include <algorithm>
#include <iostream>
#include <vector>

int main() {
    std::vector<std::vector<int>> eigenvalues(3);

    for (auto& playerValues: eigenvalues) {
        int valueCount = 0;
        std::cin >> valueCount;

        playerValues.resize(valueCount);
        for (int& value: playerValues) {
            std::cin >> value;
        }
    }

    int first = 0;
    int second = 0;
    int third = 0;
    int commonCount = 0;

    while (first < static_cast<int>(eigenvalues[0].size()) &&
           second < static_cast<int>(eigenvalues[1].size()) &&
           third < static_cast<int>(eigenvalues[2].size())) {
        const int firstValue = eigenvalues[0][first];
        const int secondValue = eigenvalues[1][second];
        const int thirdValue = eigenvalues[2][third];

        if (firstValue == secondValue && secondValue == thirdValue) {
            ++commonCount;
            ++first;
            ++second;
            ++third;
        } else {
            // двигаем индекс списка с минимальным значением в отсортированных по возрастанию (по условию) векторах
            const int minValue = std::min(firstValue, std::min(secondValue, thirdValue));
            if (firstValue == minValue) {
                ++first;
            }
            if (secondValue == minValue) {
                ++second;
            }
            if (thirdValue == minValue) {
                ++third;
            }
        }
    }

    std::cout << commonCount << '\n';

    return 0;
}
