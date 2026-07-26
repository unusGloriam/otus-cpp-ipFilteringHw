#pragma once

#include <vector>
#include <string>

class LogisticRegression {
private:
    std::vector<std::vector<double>> weights; // 10 x 785
    int num_classes = 10;
    int num_features = 784; // 28x28

public:
    LogisticRegression() = default;
    
    // Загрузка весов из файла
    bool loadWeights(const std::string& filename);
    
    // Предсказание для одного образца
    int predict(const std::vector<double>& features) const;
    
    // Вычисление точности на датасете
    double accuracy(const std::vector<std::vector<double>>& X,
                   const std::vector<int>& y) const;
    
private:
    // Сигмоидная функция
    double sigmoid(double x) const;
    
    // Softmax (для многоклассовой классификации)
    std::vector<double> softmax(const std::vector<double>& scores) const;
};