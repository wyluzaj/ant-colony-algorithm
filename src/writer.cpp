#include "writer.h"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <numeric>
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

    double mean(const std::vector<double>& values) {
        if (values.empty()) {
            return 0.0;
        }
        const double sum = std::accumulate(values.begin(), values.end(), 0.0);
        return sum / static_cast<double>(values.size());
    }

    double median(std::vector<double> values) {
        if (values.empty()) {
            return 0.0;
        }

        std::sort(values.begin(), values.end());
        const size_t middle = values.size() / 2;

        if (values.size() % 2 == 1) {
            return values[middle];
        }

        return (values[middle - 1] + values[middle]) / 2.0;
    }

    double minValue(const std::vector<double>& values) {
        if (values.empty()) {
            return 0.0;
        }
        return *std::min_element(values.begin(), values.end());
    }

    double maxValue(const std::vector<double>& values) {
        if (values.empty()) {
            return 0.0;
        }
        return *std::max_element(values.begin(), values.end());
    }

    void writeAggregateCsvHeaderIfNeeded(const std::string& filePath) {
        createParentDirectoryIfNeeded(filePath);

        if (fileExistsAndIsNotEmpty(filePath)) {
            return;
        }

        std::ofstream file(filePath);
        if (!file.is_open()) {
            throw std::runtime_error("Unable to create aggregate output file: " + filePath);
        }

        file << "instance;algorithm;dimension;runs;ants;max_time_seconds;stop_on_target_error;target_error;"
             << "alpha;beta;r;initial_pheromone;effective_initial_pheromone;deposit_amount;"
             << "use_two_opt;mode;optimal_cost;"
             << "mean_cost;median_cost;best_cost;worst_cost;"
             << "mean_time_ms;median_time_ms;best_time_ms;worst_time_ms;"
             << "mean_relative_error_percent;median_relative_error_percent;best_relative_error_percent;worst_relative_error_percent;"
             << "mean_iterations;median_iterations;best_iterations;worst_iterations;"
             << "target_success_count;target_success_rate_percent\n";
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

    file << "instance;algorithm;dimension;run_number;seed;best_cost;optimal_cost;relative_error_percent;"
         << "time_ms;iterations;stop_reason;ants;runs;max_time_seconds;stop_on_target_error;target_error;"
         << "alpha;beta;r;initial_pheromone;effective_initial_pheromone;"
         << "deposit_amount;use_two_opt;mode;path\n";
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
         << result.runNumber << ';'
         << result.seed << ';'
         << result.bestCost << ';'
         << result.optimalCost << ';'
         << result.relativeError << ';'
         << result.timeMs << ';'
         << result.iterations << ';'
         << result.stopReason << ';'
         << result.ants << ';'
         << result.runs << ';'
         << result.maxTimeSeconds << ';'
         << result.stopOnTargetError << ';'
         << result.targetError << ';'
         << result.alpha << ';'
         << result.beta << ';'
         << result.r << ';'
         << result.initialPheromone << ';'
         << result.effectiveInitialPheromone << ';'
         << result.depositAmount << ';'
         << result.useTwoOpt << ';'
         << result.mode << ';'
         << pathToCompactString(result.bestPath) << '\n';
}

void appendAggregateToCsv(const std::string& filePath, const std::vector<AlgorithmResult>& results) {
    if (results.empty()) {
        return;
    }

    createParentDirectoryIfNeeded(filePath);
    writeAggregateCsvHeaderIfNeeded(filePath);

    std::ofstream file(filePath, std::ios::app);
    if (!file.is_open()) {
        throw std::runtime_error("Unable to create aggregate output file: " + filePath);
    }

    std::vector<double> costs;
    std::vector<double> times;
    std::vector<double> errors;
    std::vector<double> iterations;

    costs.reserve(results.size());
    times.reserve(results.size());
    errors.reserve(results.size());
    iterations.reserve(results.size());

    int targetSuccessCount = 0;
    for (const AlgorithmResult& result : results) {
        costs.push_back(static_cast<double>(result.bestCost));
        times.push_back(result.timeMs);
        errors.push_back(result.relativeError);
        iterations.push_back(static_cast<double>(result.iterations));

        if (result.relativeError >= 0.0 && result.relativeError <= result.targetError) {
            ++targetSuccessCount;
        }
    }

    const AlgorithmResult& first = results.front();
    const double targetSuccessRate = 100.0 * static_cast<double>(targetSuccessCount) / static_cast<double>(results.size());

    file << first.instanceName << ';'
         << first.algorithmName << ';'
         << first.dimension << ';'
         << results.size() << ';'
         << first.ants << ';'
         << first.maxTimeSeconds << ';'
         << first.stopOnTargetError << ';'
         << first.targetError << ';'
         << first.alpha << ';'
         << first.beta << ';'
         << first.r << ';'
         << first.initialPheromone << ';'
         << first.effectiveInitialPheromone << ';'
         << first.depositAmount << ';'
         << first.useTwoOpt << ';'
         << first.mode << ';'
         << first.optimalCost << ';'
         << mean(costs) << ';'
         << median(costs) << ';'
         << minValue(costs) << ';'
         << maxValue(costs) << ';'
         << mean(times) << ';'
         << median(times) << ';'
         << minValue(times) << ';'
         << maxValue(times) << ';'
         << mean(errors) << ';'
         << median(errors) << ';'
         << minValue(errors) << ';'
         << maxValue(errors) << ';'
         << mean(iterations) << ';'
         << median(iterations) << ';'
         << minValue(iterations) << ';'
         << maxValue(iterations) << ';'
         << targetSuccessCount << ';'
         << targetSuccessRate << '\n';
}

void writeHistoryToCsv(const std::string& filePath, const std::vector<ACOHistoryEntry>& history) {
    createParentDirectoryIfNeeded(filePath);

    std::ofstream file(filePath);
    if (!file.is_open()) {
        throw std::runtime_error("Unable to create history output file: " + filePath);
    }

    file << "iteration;time_ms;best_cost;relative_error_percent\n";

    for (const ACOHistoryEntry& entry : history) {
        file << entry.iteration << ';'
             << entry.timeMs << ';'
             << entry.bestCost << ';'
             << entry.relativeError << '\n';
    }
}
