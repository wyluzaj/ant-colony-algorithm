#include "ant_colony.h"
#include "config.h"
#include "reader.h"
#include "utils.h"
#include "writer.h"

#include <exception>
#include <iostream>
#include <random>
#include <string>
#include <utility>
#include <vector>

namespace {
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
        validateConfigAfterInstanceLoad(config, instance);

        const auto optimalValues = readOptimalValues(config.optimalPath);
        const int optimalCost = findOptimalCost(optimalValues, config.instancePath, instance.name);

        if (optimalCost <= 0) {
            throw std::runtime_error(
                    "Optimal cost was not found in optimalPath file for instance: " + instance.name
            );
        }

        const int antsCount = config.aco.ants <= 0 ? instance.dimension : config.aco.ants;

        std::cout << "Instance: " << instance.name << "\n";
        std::cout << "Path: " << config.instancePath << "\n";
        std::cout << "Dimension: " << instance.dimension << "\n";
        std::cout << "Optimal cost: " << optimalCost << "\n";
        std::cout << "Ants: " << antsCount << "\n";
        std::cout << "Runs: " << config.aco.runs << "\n";
        std::cout << "Seed: "
                  << (config.aco.seed == 0 ? std::string("random for each run") : std::to_string(config.aco.seed))
                  << "\n";
        std::cout << "Mode: " << pheromoneUpdateModeToString(config.aco.mode) << "\n";
        std::cout << "2-opt local search: " << (config.aco.useTwoOpt ? "enabled" : "disabled") << "\n";
        std::cout << "Results directory: " << config.resultsDirectory << "\n";

        std::vector<AlgorithmResult> runResults;
        runResults.reserve(config.aco.runs);

        std::random_device randomDevice;

        for (int runIndex = 0; runIndex < config.aco.runs; ++runIndex) {
            const int runNumber = runIndex + 1;
            const unsigned int runSeed = seedForRun(config.aco.seed, runIndex, randomDevice);

            std::cout << "\nRun " << runNumber << "/" << config.aco.runs << ", seed: " << runSeed << "\n";

            ACOSolution solution = runAntColony(instance, config.aco, optimalCost, runSeed);

            AlgorithmResult result;
            result.instanceName = instance.name;
            result.algorithmName = "ACO";
            result.dimension = instance.dimension;

            result.runNumber = runNumber;
            result.seed = runSeed;

            result.bestCost = solution.cost;
            result.optimalCost = optimalCost;
            result.relativeError = solution.relativeError;
            result.timeMs = solution.timeMs;
            result.iterations = solution.iterations;
            result.stopReason = solution.stopReason;

            result.ants = antsCount;
            result.runs = config.aco.runs;
            result.maxTimeSeconds = config.aco.maxTimeSeconds;
            result.stopOnTargetError = config.aco.stopOnTargetError;
            result.targetError = config.aco.targetError;

            result.alpha = config.aco.alpha;
            result.beta = config.aco.beta;
            result.r = config.aco.r;
            result.initialPheromone = config.aco.initialPheromone;
            result.effectiveInitialPheromone = solution.effectiveInitialPheromone;
            result.depositAmount = config.aco.depositAmount;
            result.useTwoOpt = config.aco.useTwoOpt;

            result.mode = pheromoneUpdateModeToString(config.aco.mode);
            result.bestPath = solution.path;

            const std::string historyPath = writeHistoryToCsv(
                    config.resultsDirectory,
                    result,
                    solution.history
            );

            appendResultToCsv(config.resultsDirectory, result);
            runResults.push_back(std::move(result));

            std::cout << "Best cost: " << solution.cost << "\n";
            std::cout << "Relative error [%]: " << solution.relativeError << "\n";
            std::cout << "Time [ms]: " << solution.timeMs << "\n";
            std::cout << "Iterations: " << solution.iterations << "\n";
            std::cout << "Stop reason: " << solution.stopReason << "\n";
            std::cout << "Path: " << pathToString(solution.path, instance) << "\n";
            std::cout << "History saved to: " << historyPath << "\n";
        }

        appendAggregateToCsv(config.resultsDirectory, runResults);

        std::cout << "\nSaved detailed results under: "
                  << config.resultsDirectory << "/wyniki\n";
        std::cout << "Saved aggregate summary under: "
                  << config.resultsDirectory << "/aggregate\n";
        std::cout << "Saved history logs under: "
                  << config.resultsDirectory << "/history\n";
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }

    return 0;
}