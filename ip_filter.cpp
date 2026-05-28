#include <iostream>
#include <string>
#include <vector>
#include <algorithm>
#include <cstdint>

#include "ip_filter_utils.h"

// ("",  '.') -> [""]
// ("11", '.') -> ["11"]
// ("..", '.') -> ["", "", ""]
// ("11.", '.') -> ["11", ""]
// (".11", '.') -> ["", "11"]
// ("11.22", '.') -> ["11", "22"]
std::vector<std::string> split(const std::string &str, char d) {
    std::vector<std::string> r;

    std::string::size_type start = 0;
    std::string::size_type stop = str.find_first_of(d);
    while (stop != std::string::npos) {
        r.push_back(str.substr(start, stop - start));

        start = stop + 1;
        stop = str.find_first_of(d, start);
    }

    r.push_back(str.substr(start));

    return r;
}

int main(int, char const **) {
    try {
        std::vector<std::vector<std::string> > ip_pool;

        for (std::string line; std::getline(std::cin, line);) {
            std::vector<std::string> v = split(line, '\t');
            ip_pool.push_back(split(v.at(0), '.'));
        }

        // reverse lexicographically sort
        std::sort(ip_pool.begin(), ip_pool.end(), [](const auto &lhs_ip_address, const auto &rhs_ip_address) {
            return std::vector<int>{
                       std::stoi(lhs_ip_address.at(0)), std::stoi(lhs_ip_address.at(1)),
                       std::stoi(lhs_ip_address.at(2)), std::stoi(lhs_ip_address.at(3))
                   }
                   > std::vector<int>{
                       std::stoi(rhs_ip_address.at(0)), std::stoi(rhs_ip_address.at(1)),
                       std::stoi(rhs_ip_address.at(2)), std::stoi(rhs_ip_address.at(3))
                   };
        });

        for (const auto &ip_address: ip_pool) {
            std::cout << ip_address.at(0) << '.' << ip_address.at(1) << '.' << ip_address.at(2) << '.' << ip_address.
                    at(3) << '\n';
        }

        // TODO filter by first byte and output
        auto filtered_ip_pool = filter(ip_pool, std::false_type{}, 1);
        for (const auto &ip_address: filtered_ip_pool) {
            std::cout << ip_address.at(0) << '.' << ip_address.at(1) << '.' << ip_address.at(2) << '.' << ip_address.
                    at(3) << '\n';
        }

        // 1.231.69.33
        // 1.87.203.225
        // 1.70.44.170
        // 1.29.168.152
        // 1.1.234.8

        filtered_ip_pool = filter(ip_pool, std::false_type{}, 46, 70);
        for (const auto &ip_address: filtered_ip_pool) {
            std::cout << ip_address.at(0) << '.' << ip_address.at(1) << '.' << ip_address.at(2) << '.' << ip_address.
                    at(3) << '\n';
        }

        // 46.70.225.39
        // 46.70.147.26
        // 46.70.113.73
        // 46.70.29.76

        filtered_ip_pool = filter(ip_pool, std::true_type{}, 46);
        for (const auto &ip_address: filtered_ip_pool) {
            std::cout << ip_address.at(0) << '.' << ip_address.at(1) << '.' << ip_address.at(2) << '.' << ip_address.
                    at(3) << '\n';
        }

        // 186.204.34.46
        // 186.46.222.194
        // 185.46.87.231
        // 185.46.86.132
        // 185.46.86.131
        // 185.46.86.131
        // 185.46.86.22
        // 185.46.85.204
        // 185.46.85.78
        // 68.46.218.208
        // 46.251.197.23
        // 46.223.254.56
        // 46.223.254.56
        // 46.182.19.219
        // 46.161.63.66
        // 46.161.61.51
        // 46.161.60.92
        // 46.161.60.35
        // 46.161.58.202
        // 46.161.56.241
        // 46.161.56.203
        // 46.161.56.174
        // 46.161.56.106
        // 46.161.56.106
        // 46.101.163.119
        // 46.101.127.145
        // 46.70.225.39
        // 46.70.147.26
        // 46.70.113.73
        // 46.70.29.76
        // 46.55.46.98
        // 46.49.43.85
        // 39.46.86.85
        // 5.189.203.46
    } catch (const std::exception &e) {
        std::cerr << e.what() << std::endl;
    }

    return 0;
}
