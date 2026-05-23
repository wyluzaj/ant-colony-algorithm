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
            PheromoneMatrix& pheromones,
            int from,
            int to,
            double delta
    ) {
        validateCityIndex(instance, from);
        validateCityIndex(instance, to);

        pheromones.ref(from, to) += static_cast<float>(delta);
        if (instance.symmetric) {
            pheromones.ref(to, from) += static_cast<float>(delta);
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
                "CAS updates pheromone after a completed tour. Use depositTiming = afterTour for mode = CAS."
        );
    }

    if ((mode == PheromoneUpdateMode::DAS || mode == PheromoneUpdateMode::QAS) &&
        timing == PheromoneDepositTiming::AfterTour) {
        throw std::runtime_error(
                "DAS and QAS update pheromone after each move. Use depositTiming = afterMove for mode = DAS/QAS."
        );
    }
}

void evaporatePheromones(PheromoneMatrix& pheromones, double evaporationRate) {
    for (float& value : pheromones.values) {
        value *= static_cast<float>(1.0 - evaporationRate);
        if (value < 1e-12f) {
            value = 1e-12f;
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
    validateCityIndex(instance, from);
    validateCityIndex(instance, to);

    switch (mode) {
        case PheromoneUpdateMode::DAS:
            return depositAmount;
        case PheromoneUpdateMode::QAS: {
            const int distance = instance.distance(from, to);
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

    return 0.0;
}

void depositPheromoneOnEdge(
        const TSPInstance& instance,
        PheromoneMatrix& pheromones,
        int from,
        int to,
        int tourCost,
        double depositAmount,
        PheromoneUpdateMode mode
) {
    const double delta = calculatePheromoneDeltaForEdge(instance, from, to, tourCost, depositAmount, mode);
    addPheromone(instance, pheromones, from, to, delta);
}

void depositPheromoneOnTour(
        const TSPInstance& instance,
        PheromoneMatrix& pheromones,
        const AntTour& antTour,
        double depositAmount,
        PheromoneUpdateMode mode
) {
    if (antTour.path.empty()) {
        return;
    }

    for (size_t i = 0; i + 1 < antTour.path.size(); ++i) {
        depositPheromoneOnEdge(
                instance,
                pheromones,
                antTour.path[i],
                antTour.path[i + 1],
                antTour.cost,
                depositAmount,
                mode
        );
    }

    depositPheromoneOnEdge(
            instance,
            pheromones,
            antTour.path.back(),
            antTour.path.front(),
            antTour.cost,
            depositAmount,
            mode
    );
}
