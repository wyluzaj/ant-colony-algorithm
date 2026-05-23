#ifndef PEA4_ANT_COLONY_H
#define PEA4_ANT_COLONY_H

#include <string>
#include <vector>

#include "pheromone_update.h"
#include "tsp_instance.h"

struct ACOParameters {
    int ants = 0;                  // 0 = ants = number of cities
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

struct ACOSolution {
    std::vector<int> path;
    int cost = 0;
    int iterations = 0;
    double timeMs = 0.0;
    int optimalCost = 0;
    double relativeError = -1.0;
    std::string stopReason;
};

ACOSolution runAntColony(const TSPInstance& instance, const ACOParameters& params, int optimalCost);

double calculateRelativeError(int bestCost, int optimalCost);

#endif // PEA4_ANT_COLONY_H
