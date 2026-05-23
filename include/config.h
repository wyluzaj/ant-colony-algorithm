#ifndef PEA4_CONFIG_H
#define PEA4_CONFIG_H

#include <string>
#include <unordered_map>

#include "ant_colony.h"

struct AppConfig {
    std::string instancePath;
    std::string outputPath = "results/results.csv";
    std::string optimalPath = "config/optimal.txt";
    ACOParameters aco;
};

AppConfig readConfig(const std::string& filePath);
std::unordered_map<std::string, int> readOptimalValues(const std::string& filePath);
int findOptimalCost(
        const std::unordered_map<std::string, int>& optimalValues,
        const std::string& instancePath,
        const std::string& instanceName
);

#endif // PEA4_CONFIG_H
