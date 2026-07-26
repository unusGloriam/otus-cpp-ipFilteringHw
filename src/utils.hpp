#pragma once

#include <vector>
#include <string>

std::vector<std::vector<double>> readCSV(const std::string& filename);
void splitData(const std::vector<std::vector<double>>& data,
               std::vector<std::vector<double>>& X,
               std::vector<int>& y);