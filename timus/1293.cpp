//
// Created by miralius on 02.07.2026.
//
#include <iostream>

int main() {
    int panelCount = 0;
    int width = 0;
    int height = 0;

    std::cin >> panelCount >> width >> height;
    std::cout << 2 * panelCount * width * height << '\n';

    return 0;
}
