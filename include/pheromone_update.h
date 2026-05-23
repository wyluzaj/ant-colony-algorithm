#ifndef PEA4_PHEROMONE_UPDATE_H
#define PEA4_PHEROMONE_UPDATE_H

#include <cstddef>
#include <string>
#include <vector>

#include "tsp_instance.h"

enum class PheromoneUpdateMode {
    DAS,    // density ant system: delta = Q
    QAS,    // quantity ant system: delta = Q / d(i,j)
    CAS     // cycle ant system: delta = Q / L(k)
};

struct AntTour {
    std::vector<int> path;
    int cost = 0;
};

struct PheromoneMatrix {
    int dimension = 0;
    std::vector<float> values;

    PheromoneMatrix() = default;

    PheromoneMatrix(int n, float initialValue)
            : dimension(n), values(static_cast<std::size_t>(n) * static_cast<std::size_t>(n), initialValue) {}

    [[nodiscard]] std::size_t index(int row, int column) const {
        return static_cast<std::size_t>(row) * static_cast<std::size_t>(dimension) +
               static_cast<std::size_t>(column);
    }

    [[nodiscard]] float at(int row, int column) const {
        return values[index(row, column)];
    }

    float& ref(int row, int column) {
        return values[index(row, column)];
    }
};

std::string pheromoneUpdateModeToString(PheromoneUpdateMode mode);
PheromoneUpdateMode pheromoneUpdateModeFromString(const std::string& text);

void validatePheromoneSettings(PheromoneUpdateMode mode);

void evaporatePheromones(
        PheromoneMatrix& pheromones,
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
        PheromoneMatrix& pheromones,
        int from,
        int to,
        int tourCost,
        double depositAmount,
        PheromoneUpdateMode mode
);

void depositPheromoneOnTour(
        const TSPInstance& instance,
        PheromoneMatrix& pheromones,
        const AntTour& antTour,
        double depositAmount,
        PheromoneUpdateMode mode
);

#endif // PEA4_PHEROMONE_UPDATE_H
