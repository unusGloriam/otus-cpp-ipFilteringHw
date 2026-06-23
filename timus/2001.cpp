//
// Created by miralius on 23.06.2026.
//
#include <iostream>

int main() {
    int a1 = 0;
    int b1 = 0;
    int a2 = 0;
    int b2 = 0;
    int a3 = 0;
    int b3 = 0;

    std::cin >> a1 >> b1;
    std::cin >> a2 >> b2;
    std::cin >> a3 >> b3;

    /*
a1 = k1 + berry1
b1 = k2 + berry2

a2 = k1 + berry1 + berry2
b2 = k2

a3 = k1
b3 = k2 + berry1 + berry2
так шо berry1 = a1 - a3, berry2 = b1 - b2
     */

    std::cout << a1 - a3 << ' ' << b1 - b2 << '\n';

    return 0;
}