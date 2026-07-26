#include <vector>
#include <cmath>
#include <fstream>
#include <sstream>
#include <iostream>
#include <algorithm>

class MLP {
private:
    std::vector<std::vector<double>> w1; // 784 x 128
    std::vector<std::vector<double>> w2; // 128 x 10
    int input_size = 784;
    int hidden_size = 128;
    int output_size = 10;

public:
    bool loadWeights(const std::string& w1_file, const std::string& w2_file) {
        w1 = loadMatrix(w1_file);
        w2 = loadMatrix(w2_file);
        
        // В файлах матрицы хранятся в транспонированном виде
        // Нужно выполнить транспонирование
        w1 = transpose(w1);
        w2 = transpose(w2);
        
        if (w1.size() != input_size || w1[0].size() != hidden_size) {
            std::cerr << "Error: Invalid w1 shape" << std::endl;
            return false;
        }
        
        if (w2.size() != hidden_size || w2[0].size() != output_size) {
            std::cerr << "Error: Invalid w2 shape" << std::endl;
            return false;
        }
        
        return true;
    }
    
    int predict(const std::vector<double>& x) const {
        // Нормализуем входные данные от 0 до 1
        std::vector<double> input = x;
        
        // Первый слой: hidden = sigmoid(x * w1)
        std::vector<double> hidden(hidden_size);
        for (int j = 0; j < hidden_size; ++j) {
            double sum = 0.0;
            for (int i = 0; i < input_size; ++i) {
                sum += input[i] * w1[i][j];
            }
            hidden[j] = sigmoid(sum);
        }
        
        // Второй слой: output = softmax(hidden * w2)
        std::vector<double> scores(output_size);
        for (int j = 0; j < output_size; ++j) {
            double sum = 0.0;
            for (int i = 0; i < hidden_size; ++i) {
                sum += hidden[i] * w2[i][j];
            }
            scores[j] = sum;
        }
        
        auto probs = softmax(scores);
        return argmax(probs);
    }
    
private:
    std::vector<std::vector<double>> loadMatrix(const std::string& filename) {
        std::vector<std::vector<double>> matrix;
        std::ifstream file(filename);
        if (!file.is_open()) {
            std::cerr << "Error: Cannot open " << filename << std::endl;
            return matrix;
        }
        
        std::string line;
        while (std::getline(file, line)) {
            if (line.empty()) continue;
            
            std::vector<double> row;
            std::istringstream iss(line);
            double value;
            while (iss >> value) {
                row.push_back(value);
            }
            matrix.push_back(row);
        }
        
        return matrix;
    }
    
    std::vector<std::vector<double>> transpose(const std::vector<std::vector<double>>& matrix) {
        if (matrix.empty()) return {};
        
        std::vector<std::vector<double>> result(matrix[0].size(), 
                                                  std::vector<double>(matrix.size()));
        
        for (size_t i = 0; i < matrix.size(); ++i) {
            for (size_t j = 0; j < matrix[0].size(); ++j) {
                result[j][i] = matrix[i][j];
            }
        }
        
        return result;
    }
    
    double sigmoid(double x) const {
        return 1.0 / (1.0 + std::exp(-x));
    }
    
    std::vector<double> softmax(const std::vector<double>& scores) const {
        std::vector<double> probs(scores.size());
        double max_score = *std::max_element(scores.begin(), scores.end());
        double sum = 0.0;
        
        for (size_t i = 0; i < scores.size(); ++i) {
            probs[i] = std::exp(scores[i] - max_score);
            sum += probs[i];
        }
        
        for (auto& p : probs) {
            p /= sum;
        }
        
        return probs;
    }
    
    int argmax(const std::vector<double>& arr) const {
        auto max_it = std::max_element(arr.begin(), arr.end());
        return static_cast<int>(std::distance(arr.begin(), max_it));
    }
};