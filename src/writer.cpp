#include "writer.h"

#include <filesystem>
#include <fstream>
#include <sstream>
#include <stdexcept>

namespace {
    std::string pathToCompactString(const std::vector<int>& path) {
        std::ostringstream oss;
        for (size_t i = 0; i < path.size(); ++i) {
            if (i > 0) {
                oss << '-';
            }
            oss << path[i];
        }
        return oss.str();
    }

    void createParentDirectoryIfNeeded(const std::string& filePath) {
        const std::filesystem::path path(filePath);
        const std::filesystem::path parent = path.parent_path();

        if (!parent.empty() && !std::filesystem::exists(parent)) {
            std::filesystem::create_directories(parent);
        }
    }

    bool fileExistsAndIsNotEmpty(const std::string& filePath) {
        if (!std::filesystem::exists(filePath)) {
            return false;
        }
        return std::filesystem::file_size(filePath) > 0;
    }
}

void writeResultCsvHeaderIfNeeded(const std::string& filePath) {
    createParentDirectoryIfNeeded(filePath);

    if (fileExistsAndIsNotEmpty(filePath)) {
        return;
    }

    std::ofstream file(filePath);
    if (!file.is_open()) {
        throw std::runtime_error("Unable to create output file: " + filePath);
    }

    file << "instance;algorithm;dimension;best_cost;optimal_cost;relative_error_percent;"
         << "time_ms;iterations;stop_reason;ants;alpha;beta;r;initial_pheromone;"
         << "deposit_amount;deposit_timing;mode;path\n";
}

void appendResultToCsv(const std::string& filePath, const AlgorithmResult& result) {
    createParentDirectoryIfNeeded(filePath);
    writeResultCsvHeaderIfNeeded(filePath);

    std::ofstream file(filePath, std::ios::app);
    if (!file.is_open()) {
        throw std::runtime_error("Unable to create output file: " + filePath);
    }

    file << result.instanceName << ';'
         << result.algorithmName << ';'
         << result.dimension << ';'
         << result.bestCost << ';'
         << result.optimalCost << ';'
         << result.relativeError << ';'
         << result.timeMs << ';'
         << result.iterations << ';'
         << result.stopReason << ';'
         << result.ants << ';'
         << result.alpha << ';'
         << result.beta << ';'
         << result.r << ';'
         << result.initialPheromone << ';'
         << result.depositAmount << ';'
         << result.depositTiming << ';'
         << result.mode << ';'
         << pathToCompactString(result.bestPath) << '\n';
}
