//
// Created by miralius on 02.07.2026.
//
#include <iostream>

int main() {
    int monsterCount = 0;
    std::cin >> monsterCount;

    if (monsterCount <= 4) {
        std::cout << "few\n";
    } else if (monsterCount <= 9) {
        std::cout << "several\n";
    } else if (monsterCount <= 19) {
        std::cout << "pack\n";
    } else if (monsterCount <= 49) {
        std::cout << "lots\n";
    } else if (monsterCount <= 99) {
        std::cout << "horde\n";
    } else if (monsterCount <= 249) {
        std::cout << "throng\n";
    } else if (monsterCount <= 499) {
        std::cout << "swarm\n";
    } else if (monsterCount <= 999) {
        std::cout << "zounds\n";
    } else {
        std::cout << "legion\n";
    }

    return 0;
}
