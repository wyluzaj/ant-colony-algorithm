#include "pheromone_update.h"

#include <algorithm>
#include <cctype>
#include <stdexcept>

namespace {
    std::string normalizeText(std::string text) {
        text.erase(std::remove_if(text.begin(), text.end(), [](unsigned char c) {
            return std::isspace(c) || c == '_' || c == '-';
        }), text.end());

        std::transform(text.begin(), text.end(), text.begin(), [](unsigned char c) {
            return static_cast<char>(std::tolower(c));
        });

        return text;
    }

    void validateCityIndex(const TSPInstance& instance, int city) {
        if (city < 0 || city >= instance.dimension) {
            throw std::runtime_error("Invalid city index in ant tour.");
        }
    }

    void addPheromone(
            const TSPInstance& instance,
            std::vector<std::vector<double>>& pheromones,
            int from,
            int to,
            double delta
    ) {
        validateCityIndex(instance, from);
        validateCityIndex(instance, to);

        pheromones[from][to] += delta;
        if (instance.symmetric) {
            pheromones[to][from] += delta;
        }
    }
}

std::string pheromoneUpdateModeToString(PheromoneUpdateMode mode) {
    switch (mode) {
        case PheromoneUpdateMode::DAS:
            return "DAS";
        case PheromoneUpdateMode::QAS:
            return "QAS";
        case PheromoneUpdateMode::CAS:
            return "CAS";
    }
    return "Unknown";
}

PheromoneUpdateMode pheromoneUpdateModeFromString(const std::string& text) {
    const std::string value = normalizeText(text);

    if (value == "das" || value == "density" || value == "antdensity") {
        return PheromoneUpdateMode::DAS;
    }
    if (value == "qas" || value == "quantity" || value == "antquantity") {
        return PheromoneUpdateMode::QAS;
    }
    if (value == "cas" || value == "cycle" || value == "antcycle") {
        return PheromoneUpdateMode::CAS;
    }

    throw std::runtime_error("Unknown pheromone update mode: " + text);
}

std::string pheromoneDepositTimingToString(PheromoneDepositTiming timing) {
    switch (timing) {
        case PheromoneDepositTiming::AfterMove:
            return "AfterMove";
        case PheromoneDepositTiming::AfterTour:
            return "AfterTour";
    }
    return "Unknown";
}

PheromoneDepositTiming pheromoneDepositTimingFromString(const std::string& text) {
    const std::string value = normalizeText(text);

    if (value == "aftermove" || value == "move" || value == "edge") {
        return PheromoneDepositTiming::AfterMove;
    }
    if (value == "aftertour" || value == "tour" || value == "cycle") {
        return PheromoneDepositTiming::AfterTour;
    }

    throw std::runtime_error("Unknown pheromone deposit timing: " + text);
}

void validatePheromoneSettings(PheromoneUpdateMode mode, PheromoneDepositTiming timing) {
    if (mode == PheromoneUpdateMode::CAS && timing == PheromoneDepositTiming::AfterMove) {
        throw std::runtime_error(
                "CAS requires complete tour cost, so depositTiming must be AfterTour."
        );
    }
}

void evaporatePheromones(
        std::vector<std::vector<double>>& pheromones,
        double evaporationRate
) {
    if (evaporationRate < 0.0 || evaporationRate > 1.0) {
        throw std::runtime_error("Evaporation rate r must be in range [0, 1].");
    }

    for (auto& row : pheromones) {
        for (double& value : row) {
            value *= (1.0 - evaporationRate);
        }
    }
}

double calculatePheromoneDeltaForEdge(
        const TSPInstance& instance,
        int from,
        int to,
        int tourCost,
        double depositAmount,
        PheromoneUpdateMode mode
) {
    if (depositAmount <= 0.0) {
        throw std::runtime_error("depositAmount must be greater than 0.");
    }

    switch (mode) {
        case PheromoneUpdateMode::DAS:
            return depositAmount;

        case PheromoneUpdateMode::QAS: {
            const int distance = instance.distanceMatrix[from][to];
            if (distance <= 0) {
                return 0.0;
            }
            return depositAmount / static_cast<double>(distance);
        }

        case PheromoneUpdateMode::CAS:
            if (tourCost <= 0) {
                return 0.0;
            }
            return depositAmount / static_cast<double>(tourCost);
    }

    throw std::runtime_error("Unknown pheromone update mode.");
}

void depositPheromoneOnEdge(
        const TSPInstance& instance,
        std::vector<std::vector<double>>& pheromones,
        int from,
        int to,
        int tourCost,
        double depositAmount,
        PheromoneUpdateMode mode
) {
    if (static_cast<int>(pheromones.size()) != instance.dimension) {
        throw std::runtime_error("Invalid pheromone matrix size.");
    }

    const double delta = calculatePheromoneDeltaForEdge(
            instance,
            from,
            to,
            tourCost,
            depositAmount,
            mode
    );

    addPheromone(instance, pheromones, from, to, delta);
}

void depositPheromoneOnTour(
        const TSPInstance& instance,
        std::vector<std::vector<double>>& pheromones,
        const AntTour& antTour,
        double depositAmount,
        PheromoneUpdateMode mode
) {
    if (static_cast<int>(antTour.path.size()) != instance.dimension) {
        throw std::runtime_error("Ant tour must contain each city exactly once.");
    }

    for (int i = 0; i < instance.dimension; ++i) {
        const int from = antTour.path[i];
        const int to = antTour.path[(i + 1) % instance.dimension];
        depositPheromoneOnEdge(
                instance,
                pheromones,
                from,
                to,
                antTour.cost,
                depositAmount,
                mode
        );
    }
}
