//
// Created by miralius on 02.07.2026.
//
#include <iostream>
#include <string>

int main() {
    const int moves[8][2] = {
        {1, 2}, {2, 1}, {2, -1}, {1, -2},
        {-1, -2}, {-2, -1}, {-2, 1}, {-1, 2}
    };

    int testCount = 0;
    std::cin >> testCount;

    for (int i = 0; i < testCount; ++i) {
        std::string position;
        std::cin >> position;

        const int x = position[0] - 'a';
        const int y = position[1] - '1';

        int attackedSquares = 0;
        for (const auto& move: moves) {
            const int nextX = x + move[0];
            const int nextY = y + move[1];

            if (nextX >= 0 && nextX < 8 && nextY >= 0 && nextY < 8) {
                ++attackedSquares;
            }
        }

        std::cout << attackedSquares << '\n';
    }

    return 0;
}
