//
// Created by miralius on 02.07.2026.
//
#include <algorithm>
#include <iostream>

// операция + для нахождения минимума не подходит, у нас остается 4 комбинации
// a - b - c
// a - b * c
// a * b - c
// a * b * c

int applyOperation(int left, int right, char operation) {
    if (operation == '-') {
        return left - right;
    }
    return left * right;
}

int calculate(int first, int second, int third, char firstOperation, char secondOperation) {
    if (firstOperation == '*' && secondOperation != '*') {
        return applyOperation(first * second, third, secondOperation);
    }

    if (firstOperation != '*' && secondOperation == '*') {
        return applyOperation(first, second * third, firstOperation);
    }

    return applyOperation(applyOperation(first, second, firstOperation), third, secondOperation);
}

int main() {
    int first = 0;
    int second = 0;
    int third = 0;

    std::cin >> first >> second >> third;

    const int firstVariant = calculate(first, second, third, '-', '-');
    const int secondVariant = calculate(first, second, third, '-', '*');
    const int thirdVariant = calculate(first, second, third, '*', '-');
    const int fourthVariant = calculate(first, second, third, '*', '*');
    const int bestValue = std::min(
        std::min(firstVariant, secondVariant),
        std::min(thirdVariant, fourthVariant)
    );

    std::cout << bestValue << '\n';

    return 0;
}
