#include "ant_colony.h"
#include "config.h"
#include "reader.h"
#include "utils.h"
#include "writer.h"

#include <exception>
#include <iostream>
#include <string>

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

        std::cout << "Instance: " << instance.name << "\n";
        std::cout << "Path: " << config.instancePath << "\n";
        std::cout << "Dimension: " << instance.dimension << "\n";
        std::cout << "Optimal cost: " << optimalCost << "\n";
        std::cout << "Ants: " << antsCount << "\n";
        std::cout << "Mode: " << pheromoneUpdateModeToString(config.aco.mode) << "\n";
        std::cout << "Deposit timing: " << pheromoneDepositTimingToString(config.aco.depositTiming) << "\n";

        ACOSolution solution = runAntColony(instance, config.aco, optimalCost);

        std::cout << "Best cost: " << solution.cost << "\n";
        std::cout << "Relative error [%]: " << solution.relativeError << "\n";
        std::cout << "Time [ms]: " << solution.timeMs << "\n";
        std::cout << "Iterations: " << solution.iterations << "\n";
        std::cout << "Stop reason: " << solution.stopReason << "\n";
        std::cout << "Path: " << pathToString(solution.path, instance) << "\n";

        appendResultToCsv(config.outputPath, AlgorithmResult{
                instance.name,
                "ACO",
                instance.dimension,
                solution.cost,
                optimalCost,
                solution.relativeError,
                solution.timeMs,
                solution.iterations,
                solution.stopReason,
                antsCount,
                config.aco.alpha,
                config.aco.beta,
                config.aco.r,
                config.aco.initialPheromone,
                config.aco.depositAmount,
                pheromoneDepositTimingToString(config.aco.depositTiming),
                pheromoneUpdateModeToString(config.aco.mode),
                solution.path
        });

        std::cout << "Saved result to: " << config.outputPath << "\n";
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }

    return 0;
}
