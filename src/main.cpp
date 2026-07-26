#include <iostream>
#include <string>
#include <chrono>
#include <iomanip>  // <-- ДОБАВЛЯЕМ ЭТОТ ЗАГОЛОВОК
#include "logistic_regression.hpp"
#include "utils.hpp"

void printHelp() {
    std::cout << "Fashion MNIST Classifier" << std::endl;
    std::cout << std::endl;
    std::cout << "Usage:" << std::endl;
    std::cout << "  fashion_mnist <model_type> <test_file> <model_file>" << std::endl;
    std::cout << std::endl;
    std::cout << "Model types:" << std::endl;
    std::cout << "  logreg   - Logistic Regression (logreg_coef.txt)" << std::endl;
    std::cout << "  mlp      - Multi-Layer Perceptron (w1.txt, w2.txt)" << std::endl;
    std::cout << "  catboost - CatBoost model (model.cbm)" << std::endl;
    std::cout << "  cnn      - CNN model (saved_model/)" << std::endl;
    std::cout << std::endl;
    std::cout << "Example:" << std::endl;
    std::cout << "  fashion_mnist logreg test.csv logreg_coef.txt" << std::endl;
}

int main(int argc, char* argv[]) {
    if (argc != 4) {
        printHelp();
        return 1;
    }
    
    std::string model_type = argv[1];
    std::string test_file = argv[2];
    std::string model_file = argv[3];
    
    std::cout << "Loading test data from: " << test_file << std::endl;
    auto data = readCSV(test_file);
    
    if (data.empty()) {
        std::cerr << "Error: No data loaded" << std::endl;
        return 1;
    }
    
    std::vector<std::vector<double>> X;
    std::vector<int> y;
    splitData(data, X, y);
    
    std::cout << "Loaded " << X.size() << " test samples" << std::endl;
    std::cout << "Features per sample: " << X[0].size() << std::endl;
    
    auto start = std::chrono::high_resolution_clock::now();
    double acc = 0.0;
    
    if (model_type == "logreg") {
        std::cout << "Using Logistic Regression model" << std::endl;
        LogisticRegression model;
        
        if (!model.loadWeights(model_file)) {
            return 1;
        }
        
        acc = model.accuracy(X, y);
    } 
    else if (model_type == "mlp") {
        std::cout << "MLP model loading not yet implemented" << std::endl;
        std::cout << "Implement MLP inference using w1.txt and w2.txt" << std::endl;
        return 1;
    }
    else if (model_type == "catboost") {
        std::cout << "CatBoost model loading not yet implemented" << std::endl;
        std::cout << "Need to link with libcatboost.so" << std::endl;
        return 1;
    }
    else if (model_type == "cnn") {
        std::cout << "CNN model loading not yet implemented" << std::endl;
        std::cout << "Need to link with TensorFlow C API" << std::endl;
        return 1;
    }
    else {
        std::cerr << "Unknown model type: " << model_type << std::endl;
        printHelp();
        return 1;
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    
    // Исправляем вывод - убираем setprecision, используем стандартный вывод
    std::cout << "Accuracy: " << acc << std::endl;
    std::cout << "Time: " << duration.count() << " ms" << std::endl;
    
    return 0;
}