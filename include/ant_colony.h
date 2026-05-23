#ifndef PEA4_ANT_COLONY_H
#define PEA4_ANT_COLONY_H

#include <string>
#include <vector>

#include "pheromone_update.h"
#include "tsp_instance.h"

struct ACOParameters {
    int ants = 0;                  // 0 = ants = number of cities
    int runs = 1;                  // number of independent runs for one config
    unsigned int seed = 0;         // 0 = random seed for each run, otherwise seed + runIndex
    double maxTimeSeconds = 60.0;

    bool stopOnTargetError = true;
    double targetError = 20.0;     // percent

    double alpha = 1.0;            // importance of pheromone
    double beta = 2.0;             // importance of heuristic information
    double r = 0.5;                // evaporation rate

    double initialPheromone = 0.0; // 0 = auto: ants / Cnn
    double depositAmount = 1.0;    // Q used in DAS/QAS/CAS

    PheromoneDepositTiming depositTiming = PheromoneDepositTiming::AfterTour;
    PheromoneUpdateMode mode = PheromoneUpdateMode::CAS;
};

struct ACOHistoryEntry {
    int iteration = 0;
    double timeMs = 0.0;
    int bestCost = 0;
    double relativeError = -1.0;
};

struct ACOSolution {
    std::vector<int> path;
    int cost = 0;
    int iterations = 0;
    double timeMs = 0.0;
    int optimalCost = 0;
    double relativeError = -1.0;
    std::string stopReason;
    unsigned int seed = 0;
    double effectiveInitialPheromone = 0.0;
    std::vector<ACOHistoryEntry> history;
};

ACOSolution runAntColony(
        const TSPInstance& instance,
        const ACOParameters& params,
        int optimalCost,
        unsigned int seed
);

double calculateRelativeError(int bestCost, int optimalCost);

#endif // PEA4_ANT_COLONY_H
