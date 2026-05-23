#ifndef PEA4_PHEROMONE_UPDATE_H
#define PEA4_PHEROMONE_UPDATE_H

#include <string>
#include <vector>

#include "tsp_instance.h"

enum class PheromoneUpdateMode {
    DAS,    // density ant system: delta = Q
    QAS,    // quantity ant system: delta = Q / d(i,j)
    CAS     // cycle ant system: delta = Q / L(k)
};

enum class PheromoneDepositTiming {
    AfterMove,
    AfterTour
};

struct AntTour {
    std::vector<int> path;
    int cost = 0;
};

std::string pheromoneUpdateModeToString(PheromoneUpdateMode mode);
PheromoneUpdateMode pheromoneUpdateModeFromString(const std::string& text);

std::string pheromoneDepositTimingToString(PheromoneDepositTiming timing);
PheromoneDepositTiming pheromoneDepositTimingFromString(const std::string& text);

void validatePheromoneSettings(PheromoneUpdateMode mode, PheromoneDepositTiming timing);

void evaporatePheromones(
        std::vector<std::vector<double>>& pheromones,
        double evaporationRate
);

double calculatePheromoneDeltaForEdge(
        const TSPInstance& instance,
        int from,
        int to,
        int tourCost,
        double depositAmount,
        PheromoneUpdateMode mode
);

void depositPheromoneOnEdge(
        const TSPInstance& instance,
        std::vector<std::vector<double>>& pheromones,
        int from,
        int to,
        int tourCost,
        double depositAmount,
        PheromoneUpdateMode mode
);

void depositPheromoneOnTour(
        const TSPInstance& instance,
        std::vector<std::vector<double>>& pheromones,
        const AntTour& antTour,
        double depositAmount,
        PheromoneUpdateMode mode
);

#endif // PEA4_PHEROMONE_UPDATE_H
