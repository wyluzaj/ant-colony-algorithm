#include "writer.h"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <stdexcept>

namespace {
    constexpr char CSV_SEPARATOR = ',';

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

    std::string safeToken(std::string value) {
        for (char& c : value) {
            if (c == '.' || c == ',' || c == ':' || c == ';' || c == '/' || c == '\\' || c == ' ') {
                c = '_';
            }
        }
        return value;
    }

    std::string doubleToken(double value) {
        std::ostringstream oss;
        oss << std::fixed << std::setprecision(3) << value;
        return safeToken(oss.str());
    }

    std::string boolToken(bool value) {
        return value ? "true" : "false";
    }

    std::string resultFileForInstance(const std::string& resultsDirectory, const std::string& instanceName) {
        const std::filesystem::path path =
                std::filesystem::path(resultsDirectory)
                / "wyniki"
                / (safeToken(instanceName) + ".csv");

        return path.string();
    }

    std::string aggregateFilePath(const std::string& resultsDirectory) {
        const std::filesystem::path path =
                std::filesystem::path(resultsDirectory)
                / "aggregate"
                / "aco_aggregate.csv";

        return path.string();
    }

    std::string historyFileForRun(const std::string& resultsDirectory, const AlgorithmResult& result) {
        std::ostringstream fileName;

        fileName << safeToken(result.instanceName)
                 << "_run" << result.runNumber
                 << "_seed" << result.seed
                 << "_alpha" << doubleToken(result.alpha)
                 << "_beta" << doubleToken(result.beta)
                 << "_time" << doubleToken(result.maxTimeSeconds)
                 << "_target" << boolToken(result.stopOnTargetError)
                 << "_targetError" << doubleToken(result.targetError)
                 << "_twoOpt" << boolToken(result.useTwoOpt)
                 << "_mode" << safeToken(result.mode)
                 << ".csv";

        const std::filesystem::path path =
                std::filesystem::path(resultsDirectory)
                / "history"
                / safeToken(result.instanceName)
                / fileName.str();

        return path.string();
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

        file << "instance,algorithm,dimension,run_number,seed,best_cost,optimal_cost,relative_error_percent,"
             << "time_ms,iterations,stop_reason,ants,runs,max_time_seconds,stop_on_target_error,target_error,"
             << "alpha,beta,r,initial_pheromone,effective_initial_pheromone,"
             << "deposit_amount,use_two_opt,mode\n";
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

        file << "instance,algorithm,dimension,runs,ants,max_time_seconds,stop_on_target_error,target_error,"
             << "alpha,beta,r,initial_pheromone,effective_initial_pheromone,deposit_amount,"
             << "use_two_opt,mode,optimal_cost,"
             << "median_time_ms,"
             << "median_relative_error_percent,best_relative_error_percent,"
             << "median_iterations,"
             << "target_success_count\n";
    }
}

void appendResultToCsv(const std::string& resultsDirectory, const AlgorithmResult& result) {
    const std::string filePath = resultFileForInstance(resultsDirectory, result.instanceName);

    createParentDirectoryIfNeeded(filePath);
    writeResultCsvHeaderIfNeeded(filePath);

    std::ofstream file(filePath, std::ios::app);
    if (!file.is_open()) {
        throw std::runtime_error("Unable to create output file: " + filePath);
    }

    file << result.instanceName << CSV_SEPARATOR
         << result.algorithmName << CSV_SEPARATOR
         << result.dimension << CSV_SEPARATOR
         << result.runNumber << CSV_SEPARATOR
         << result.seed << CSV_SEPARATOR
         << result.bestCost << CSV_SEPARATOR
         << result.optimalCost << CSV_SEPARATOR
         << result.relativeError << CSV_SEPARATOR
         << result.timeMs << CSV_SEPARATOR
         << result.iterations << CSV_SEPARATOR
         << result.stopReason << CSV_SEPARATOR
         << result.ants << CSV_SEPARATOR
         << result.runs << CSV_SEPARATOR
         << result.maxTimeSeconds << CSV_SEPARATOR
         << result.stopOnTargetError << CSV_SEPARATOR
         << result.targetError << CSV_SEPARATOR
         << result.alpha << CSV_SEPARATOR
         << result.beta << CSV_SEPARATOR
         << result.r << CSV_SEPARATOR
         << result.initialPheromone << CSV_SEPARATOR
         << result.effectiveInitialPheromone << CSV_SEPARATOR
         << result.depositAmount << CSV_SEPARATOR
         << result.useTwoOpt << CSV_SEPARATOR
         << result.mode << '\n';
}

void appendAggregateToCsv(const std::string& resultsDirectory, const std::vector<AlgorithmResult>& results) {
    if (results.empty()) {
        return;
    }

    const std::string filePath = aggregateFilePath(resultsDirectory);

    createParentDirectoryIfNeeded(filePath);
    writeAggregateCsvHeaderIfNeeded(filePath);

    std::ofstream file(filePath, std::ios::app);
    if (!file.is_open()) {
        throw std::runtime_error("Unable to create aggregate output file: " + filePath);
    }

    std::vector<double> times;
    std::vector<double> errors;
    std::vector<double> iterations;

    times.reserve(results.size());
    errors.reserve(results.size());
    iterations.reserve(results.size());

    int targetSuccessCount = 0;
    for (const AlgorithmResult& result : results) {
        times.push_back(result.timeMs);
        errors.push_back(result.relativeError);
        iterations.push_back(static_cast<double>(result.iterations));

        if (result.relativeError >= 0.0 && result.relativeError <= result.targetError) {
            ++targetSuccessCount;
        }
    }

    const AlgorithmResult& first = results.front();
    const double targetSuccessRate =
            100.0 * static_cast<double>(targetSuccessCount) / static_cast<double>(results.size());

    file << first.instanceName << CSV_SEPARATOR
         << first.algorithmName << CSV_SEPARATOR
         << first.dimension << CSV_SEPARATOR
         << results.size() << CSV_SEPARATOR
         << first.ants << CSV_SEPARATOR
         << first.maxTimeSeconds << CSV_SEPARATOR
         << first.stopOnTargetError << CSV_SEPARATOR
         << first.targetError << CSV_SEPARATOR
         << first.alpha << CSV_SEPARATOR
         << first.beta << CSV_SEPARATOR
         << first.r << CSV_SEPARATOR
         << first.initialPheromone << CSV_SEPARATOR
         << first.effectiveInitialPheromone << CSV_SEPARATOR
         << first.depositAmount << CSV_SEPARATOR
         << first.useTwoOpt << CSV_SEPARATOR
         << first.mode << CSV_SEPARATOR
         << first.optimalCost << CSV_SEPARATOR
         << median(times) << CSV_SEPARATOR
         << median(errors) << CSV_SEPARATOR
         << minValue(errors) << CSV_SEPARATOR
         << median(iterations) << CSV_SEPARATOR
         << targetSuccessCount <<'\n';
}

std::string writeHistoryToCsv(
        const std::string& resultsDirectory,
        const AlgorithmResult& result,
        const std::vector<ACOHistoryEntry>& history
) {
    const std::string filePath = historyFileForRun(resultsDirectory, result);

    createParentDirectoryIfNeeded(filePath);

    std::ofstream file(filePath);
    if (!file.is_open()) {
        throw std::runtime_error("Unable to create history output file: " + filePath);
    }

    file << "iteration,time_ms,best_cost,relative_error_percent\n";

    for (const ACOHistoryEntry& entry : history) {
        file << entry.iteration << CSV_SEPARATOR
             << entry.timeMs << CSV_SEPARATOR
             << entry.bestCost << CSV_SEPARATOR
             << entry.relativeError << '\n';
    }

    return filePath;
}