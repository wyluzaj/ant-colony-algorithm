#include "ant_colony.h"
#include "config.h"
#include "reader.h"
#include "utils.h"
#include "writer.h"

#include <exception>
#include <filesystem>
#include <iostream>
#include <random>
#include <string>
#include <vector>

namespace {
    std::string resultFileForInstance(const std::string& resultsDirectory, const std::string& instanceName) {
        const std::filesystem::path path = std::filesystem::path(resultsDirectory) / "wyniki" / (instanceName + ".csv");
        return path.string();
    }

    std::string aggregateFilePath(const std::string& resultsDirectory) {
        const std::filesystem::path path = std::filesystem::path(resultsDirectory) / "aggregate" / "aco_aggregate.csv";
        return path.string();
    }

    unsigned int seedForRun(unsigned int configuredSeed, int runIndex, std::random_device& randomDevice) {
        if (configuredSeed == 0) {
            return randomDevice();
        }
        return configuredSeed + static_cast<unsigned int>(runIndex);
    }
}

int main(int argc, char** argv) {
    try {
        if (argc < 2) {
            std::cout << "Usage: ./aco_tsp <config_file>\n";
            std::cout << "Example: ./aco_tsp config/aco_config.txt\n";
            return 0;
        }

        const std::string configPath = normalizePath(argv[1]);
        AppConfig config = readConfig(configPath);

        TSPInstance instance = readTSPInstance(config.instancePath);
        const auto optimalValues = readOptimalValues(config.optimalPath);
        const int optimalCost = findOptimalCost(optimalValues, config.instancePath, instance.name);

        if (optimalCost <= 0) {
            throw std::runtime_error(
                    "Optimal cost was not found in optimalPath file for instance: " + instance.name
            );
        }

        const int antsCount = config.aco.ants <= 0 ? instance.dimension : config.aco.ants;
        const std::string instanceResultPath = resultFileForInstance(config.resultsDirectory, instance.name);
        const std::string aggregatePath = aggregateFilePath(config.resultsDirectory);

        std::cout << "Instance: " << instance.name << "\n";
        std::cout << "Path: " << config.instancePath << "\n";
        std::cout << "Dimension: " << instance.dimension << "\n";
        std::cout << "Optimal cost: " << optimalCost << "\n";
        std::cout << "Ants: " << antsCount << "\n";
        std::cout << "Runs: " << config.aco.runs << "\n";
        std::cout << "Seed: " << (config.aco.seed == 0 ? std::string("random for each run") : std::to_string(config.aco.seed)) << "\n";
        std::cout << "Mode: " << pheromoneUpdateModeToString(config.aco.mode) << "\n";
        std::cout << "Deposit timing: " << pheromoneDepositTimingToString(config.aco.depositTiming) << "\n";

        std::vector<AlgorithmResult> runResults;
        runResults.reserve(config.aco.runs);
        std::random_device randomDevice;

        for (int runIndex = 0; runIndex < config.aco.runs; ++runIndex) {
            const int runNumber = runIndex + 1;
            const unsigned int runSeed = seedForRun(config.aco.seed, runIndex, randomDevice);

            std::cout << "\nRun " << runNumber << "/" << config.aco.runs << ", seed: " << runSeed << "\n";

            ACOSolution solution = runAntColony(instance, config.aco, optimalCost, runSeed);

            std::cout << "Best cost: " << solution.cost << "\n";
            std::cout << "Relative error [%]: " << solution.relativeError << "\n";
            std::cout << "Time [ms]: " << solution.timeMs << "\n";
            std::cout << "Iterations: " << solution.iterations << "\n";
            std::cout << "Stop reason: " << solution.stopReason << "\n";
            std::cout << "Path: " << pathToString(solution.path, instance) << "\n";

            AlgorithmResult result{
                    instance.name,
                    "ACO",
                    instance.dimension,
                    runNumber,
                    runSeed,
                    solution.cost,
                    optimalCost,
                    solution.relativeError,
                    solution.timeMs,
                    solution.iterations,
                    solution.stopReason,
                    antsCount,
                    config.aco.runs,
                    config.aco.maxTimeSeconds,
                    config.aco.stopOnTargetError,
                    config.aco.targetError,
                    config.aco.alpha,
                    config.aco.beta,
                    config.aco.r,
                    config.aco.initialPheromone,
                    solution.effectiveInitialPheromone,
                    config.aco.depositAmount,
                    pheromoneDepositTimingToString(config.aco.depositTiming),
                    pheromoneUpdateModeToString(config.aco.mode),
                    solution.path
            };

            appendResultToCsv(instanceResultPath, result);
            appendResultToCsv(config.outputPath, result);
            runResults.push_back(std::move(result));
        }

        appendAggregateToCsv(aggregatePath, runResults);

        std::cout << "\nSaved run results to: " << instanceResultPath << "\n";
        std::cout << "Saved aggregate summary to: " << aggregatePath << "\n";
        std::cout << "Saved backward-compatible CSV to: " << config.outputPath << "\n";
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }

    return 0;
}
