#include "logistic_regression.hpp"
#include <fstream>
#include <sstream>
#include <cmath>
#include <iostream>
#include <algorithm>
#include <numeric>
#include <iomanip>  // <-- ДОБАВЛЯЕМ ДЛЯ setprecision

bool LogisticRegression::loadWeights(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Error: Cannot open " << filename << std::endl;
        return false;
    }
    
    weights.clear();
    std::string line;
    
    while (std::getline(file, line)) {
        if (line.empty()) continue;
        
        std::vector<double> row;
        std::istringstream iss(line);
        double value;
        
        while (iss >> value) {
            row.push_back(value);
        }
        
        if (row.size() != 785) {
            std::cerr << "Error: Invalid row size: " << row.size() 
                      << " (expected 785)" << std::endl;
            return false;
        }
        
        weights.push_back(row);
    }
    
    if (weights.size() != num_classes) {
        std::cerr << "Error: Expected " << num_classes 
                  << " rows, got " << weights.size() << std::endl;
        return false;
    }
    
    std::cout << "Loaded " << weights.size() << " x " << weights[0].size() 
              << " weights matrix" << std::endl;
    return true;
}

double LogisticRegression::sigmoid(double x) const {
    return 1.0 / (1.0 + std::exp(-x));
}

std::vector<double> LogisticRegression::softmax(const std::vector<double>& scores) const {
    std::vector<double> probs(scores.size());
    double max_score = *std::max_element(scores.begin(), scores.end());
    double sum = 0.0;
    
    for (size_t i = 0; i < scores.size(); ++i) {
        probs[i] = std::exp(scores[i] - max_score);
        sum += probs[i];
    }
    
    if (sum > 0.0) {
        for (auto& p : probs) {
            p /= sum;
        }
    }
    
    return probs;
}

int LogisticRegression::predict(const std::vector<double>& features) const {
    if (features.size() != num_features) {
        std::cerr << "Error: Expected " << num_features 
                  << " features, got " << features.size() << std::endl;
        return -1;
    }
    
    std::vector<double> scores(num_classes);
    
    // Для каждого класса вычисляем: w0 + w1*x1 + w2*x2 + ... + w784*x784
    for (int c = 0; c < num_classes; ++c) {
        double score = weights[c][0]; // bias (свободный член)
        
        for (int i = 0; i < num_features; ++i) {
            score += weights[c][i + 1] * features[i];
        }
        
        scores[c] = score;
    }
    
    // Применяем softmax для получения вероятностей
    std::vector<double> probs = softmax(scores);
    
    // Находим класс с максимальной вероятностью
    auto max_it = std::max_element(probs.begin(), probs.end());
    return static_cast<int>(std::distance(probs.begin(), max_it));
}

double LogisticRegression::accuracy(const std::vector<std::vector<double>>& X,
                                    const std::vector<int>& y) const {
    if (X.size() != y.size()) {
        std::cerr << "Error: X and y sizes don't match" << std::endl;
        return 0.0;
    }
    
    size_t correct = 0;
    size_t total = X.size();
    
    for (size_t i = 0; i < total; ++i) {
        int pred = predict(X[i]);
        if (pred == y[i]) {
            correct++;
        }
        
        // Выводим прогресс каждые 1000 образцов
        if ((i + 1) % 1000 == 0) {
            std::cout << "Processed " << (i + 1) << "/" << total 
                      << " samples..." << std::endl;
        }
    }
    
    return static_cast<double>(correct) / total;
}