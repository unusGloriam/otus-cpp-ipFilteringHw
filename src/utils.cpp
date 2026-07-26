#include "utils.hpp"
#include <fstream>
#include <sstream>
#include <iostream>
#include <cctype>

std::vector<std::vector<double>> readCSV(const std::string& filename) {
    std::vector<std::vector<double>> data;
    std::ifstream file(filename);
    
    if (!file.is_open()) {
        std::cerr << "Error: Cannot open " << filename << std::endl;
        return data;
    }
    
    std::string line;
    bool is_first = true;
    
    while (std::getline(file, line)) {
        if (line.empty()) continue;
        
        // Пропускаем заголовок
        if (is_first) {
            // Проверяем, есть ли буквы в первой строке
            bool has_letters = false;
            for (char c : line) {
                if (std::isalpha(c)) {
                    has_letters = true;
                    break;
                }
            }
            if (has_letters) {
                is_first = false;
                continue;
            }
            is_first = false;
        }
        
        std::vector<double> row;
        std::istringstream iss(line);
        std::string token;
        
        // Читаем метку класса
        if (!std::getline(iss, token, ',')) continue;
        if (token.empty()) continue;
        
        try {
            int label = std::stoi(token);
            row.push_back(static_cast<double>(label));
        } catch (...) {
            continue;
        }
        
        // Читаем пиксели
        while (std::getline(iss, token, ',')) {
            if (!token.empty()) {
                try {
                    double pixel = std::stod(token);
                    row.push_back(pixel);
                } catch (...) {
                    row.push_back(0.0);
                }
            }
        }
        
        // Проверяем, что у нас 785 значений (1 метка + 784 пикселя)
        if (row.size() == 785) {
            data.push_back(row);
        }
    }
    
    std::cout << "Loaded " << data.size() << " rows from " << filename << std::endl;
    return data;
}

void splitData(const std::vector<std::vector<double>>& data,
               std::vector<std::vector<double>>& X,
               std::vector<int>& y) {
    X.clear();
    y.clear();
    
    for (const auto& row : data) {
        if (row.size() < 2) continue;
        
        y.push_back(static_cast<int>(row[0]));
        std::vector<double> features(row.begin() + 1, row.end());
        X.push_back(features);
    }
}