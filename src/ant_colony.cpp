#include "ant_colony.h"

#include "utils.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <limits>
#include <numeric>
#include <random>
#include <stdexcept>

namespace {
    std::vector<int> nearestNeighborPath(const TSPInstance& instance, int startCity) {
        const int n = instance.dimension;
        std::vector<int> path;
        std::vector<bool> visited(n, false);
        path.reserve(n);

        int current = startCity;
        path.push_back(current);
        visited[current] = true;

        while (static_cast<int>(path.size()) < n) {
            int bestCity = -1;
            int bestDistance = std::numeric_limits<int>::max();

            for (int candidate = 0; candidate < n; ++candidate) {
                if (!visited[candidate] && instance.distance(current, candidate) < bestDistance) {
                    bestDistance = instance.distance(current, candidate);
                    bestCity = candidate;
                }
            }

            if (bestCity == -1) {
                throw std::runtime_error("Nearest neighbor failed to build a complete path.");
            }

            current = bestCity;
            visited[current] = true;
            path.push_back(current);
        }

        return path;
    }

    int calculateBestNearestNeighborCost(const TSPInstance& instance) {
        int bestCost = std::numeric_limits<int>::max();
        for (int start = 0; start < instance.dimension; ++start) {
            const std::vector<int> path = nearestNeighborPath(instance, start);
            bestCost = std::min(bestCost, calculatePathCost(instance, path));
        }
        return bestCost;
    }

    double calculateEffectiveInitialPheromone(
            const TSPInstance& instance,
            int antsCount,
            double configuredInitialPheromone
    ) {
        if (configuredInitialPheromone > 0.0) {
            return configuredInitialPheromone;
        }

        const int cnn = calculateBestNearestNeighborCost(instance);
        return static_cast<double>(antsCount) / static_cast<double>(cnn);
    }

    PheromoneMatrix initializePheromones(
            const TSPInstance& instance,
            double effectiveInitialPheromone
    ) {
        PheromoneMatrix pheromones(instance.dimension, static_cast<float>(effectiveInitialPheromone));

        for (int i = 0; i < instance.dimension; ++i) {
            pheromones.ref(i, i) = 0.0f;
        }

        return pheromones;
    }

    int chooseNextCity(
            const TSPInstance& instance,
            const PheromoneMatrix& pheromones,
            const std::vector<bool>& visited,
            int currentCity,
            double alpha,
            double beta,
            std::mt19937& rng
    ) {
        std::vector<int> candidates;
        std::vector<double> weights;
        candidates.reserve(instance.dimension);
        weights.reserve(instance.dimension);

        double sum = 0.0;
        for (int city = 0; city < instance.dimension; ++city) {
            if (visited[city] || city == currentCity) {
                continue;
            }

            const int distance = instance.distance(currentCity, city);
            if (distance <= 0) {
                continue;
            }

            const double tau = std::max(static_cast<double>(pheromones.at(currentCity, city)), 1e-12);
            const double eta = 1.0 / static_cast<double>(distance);
            const double weight = std::pow(tau, alpha) * std::pow(eta, beta);

            candidates.push_back(city);
            weights.push_back(weight);
            sum += weight;
        }

        if (candidates.empty()) {
            throw std::runtime_error("No candidate city available while constructing ant tour.");
        }

        if (sum <= 0.0) {
            std::uniform_int_distribution<int> distribution(0, static_cast<int>(candidates.size()) - 1);
            return candidates[distribution(rng)];
        }

        std::uniform_real_distribution<double> distribution(0.0, sum);
        double randomValue = distribution(rng);

        for (size_t i = 0; i < candidates.size(); ++i) {
            randomValue -= weights[i];
            if (randomValue <= 0.0) {
                return candidates[i];
            }
        }

        return candidates.back();
    }


    bool isSymmetricDistanceMatrix(const TSPInstance& instance);
    void applyTwoOptIfEnabled(const TSPInstance& instance, AntTour& tour, bool useTwoOpt, bool symmetricMatrix);

    AntTour buildAntTour(
            const TSPInstance& instance,
            PheromoneMatrix& pheromones,
            const ACOParameters& params,
            std::mt19937& rng,
            int startCity,
            bool symmetricMatrix
    ) {
        const int n = instance.dimension;
        AntTour tour;
        tour.path.reserve(n);

        std::vector<bool> visited(n, false);
        int currentCity = startCity;
        visited[currentCity] = true;
        tour.path.push_back(currentCity);

        while (static_cast<int>(tour.path.size()) < n) {
            const int nextCity = chooseNextCity(
                    instance,
                    pheromones,
                    visited,
                    currentCity,
                    params.alpha,
                    params.beta,
                    rng
            );

            if (params.depositTiming == PheromoneDepositTiming::AfterMove) {
                depositPheromoneOnEdge(
                        instance,
                        pheromones,
                        currentCity,
                        nextCity,
                        0,
                        params.depositAmount,
                        params.mode
                );
            }

            currentCity = nextCity;
            visited[currentCity] = true;
            tour.path.push_back(currentCity);
        }

        if (params.depositTiming == PheromoneDepositTiming::AfterMove) {
            const int from = currentCity;
            const int to = tour.path.front();

            depositPheromoneOnEdge(
                    instance,
                    pheromones,
                    from,
                    to,
                    0,
                    params.depositAmount,
                    params.mode
            );
        }

        applyTwoOptIfEnabled(instance, tour, params.useTwoOpt, symmetricMatrix);
        return tour;
    }


    bool isSymmetricDistanceMatrix(const TSPInstance& instance) {
        for (int i = 0; i < instance.dimension; ++i) {
            for (int j = i + 1; j < instance.dimension; ++j) {
                if (instance.distance(i, j) != instance.distance(j, i)) {
                    return false;
                }
            }
        }
        return true;
    }

    void twoOptImproveSymmetric(const TSPInstance& instance, AntTour& tour) {
        const int n = static_cast<int>(tour.path.size());
        if (n < 4) {
            tour.cost = calculatePathCost(instance, tour.path);
            return;
        }

        bool improved = true;
        while (improved) {
            improved = false;

            for (int i = 1; i < n - 1 && !improved; ++i) {
                for (int k = i + 1; k < n && !improved; ++k) {
                    const int a = tour.path[i - 1];
                    const int b = tour.path[i];
                    const int c = tour.path[k];
                    const int d = tour.path[(k + 1) % n];

                    const int removed = instance.distance(a, b) + instance.distance(c, d);
                    const int added = instance.distance(a, c) + instance.distance(b, d);

                    if (added < removed) {
                        std::reverse(tour.path.begin() + i, tour.path.begin() + k + 1);
                        tour.cost += added - removed;
                        improved = true;
                    }
                }
            }
        }
    }

    void twoOptImproveAsymmetricSafe(const TSPInstance& instance, AntTour& tour) {
        const int n = static_cast<int>(tour.path.size());
        if (n < 4) {
            tour.cost = calculatePathCost(instance, tour.path);
            return;
        }

        tour.cost = calculatePathCost(instance, tour.path);
        bool improved = true;
        while (improved) {
            improved = false;

            for (int i = 1; i < n - 1 && !improved; ++i) {
                for (int k = i + 1; k < n && !improved; ++k) {
                    std::reverse(tour.path.begin() + i, tour.path.begin() + k + 1);
                    const int candidateCost = calculatePathCost(instance, tour.path);

                    if (candidateCost < tour.cost) {
                        tour.cost = candidateCost;
                        improved = true;
                    } else {
                        std::reverse(tour.path.begin() + i, tour.path.begin() + k + 1);
                    }
                }
            }
        }
    }

    void applyTwoOptIfEnabled(const TSPInstance& instance, AntTour& tour, bool useTwoOpt, bool symmetricMatrix) {
        if (!useTwoOpt) {
            tour.cost = calculatePathCost(instance, tour.path);
            return;
        }

        if (symmetricMatrix) {
            tour.cost = calculatePathCost(instance, tour.path);
            twoOptImproveSymmetric(instance, tour);
        } else {
            twoOptImproveAsymmetricSafe(instance, tour);
        }
    }

    bool isTimeLimitReached(
            const std::chrono::high_resolution_clock::time_point& start,
            double maxTimeSeconds
    ) {
        const auto now = std::chrono::high_resolution_clock::now();
        return std::chrono::duration<double>(now - start).count() >= maxTimeSeconds;
    }
}

double calculateRelativeError(int bestCost, int optimalCost) {
    if (optimalCost <= 0) {
        return -1.0;
    }
    return (static_cast<double>(bestCost - optimalCost) / static_cast<double>(optimalCost)) * 100.0;
}

ACOSolution runAntColony(
        const TSPInstance& instance,
        const ACOParameters& params,
        int optimalCost,
        unsigned int seed
) {
    if (!instance.isValid()) {
        throw std::runtime_error("Incorrect TSP instance.");
    }
    validatePheromoneSettings(params.mode, params.depositTiming);

    const int antsCount = params.ants <= 0 ? instance.dimension : params.ants;
    const double effectiveInitialPheromone = calculateEffectiveInitialPheromone(
            instance,
            antsCount,
            params.initialPheromone
    );
    PheromoneMatrix pheromones = initializePheromones(
            instance,
            effectiveInitialPheromone
    );

    ACOSolution bestSolution;
    bestSolution.path = nearestNeighborPath(instance, 0);
    bestSolution.cost = calculatePathCost(instance, bestSolution.path);
    bestSolution.optimalCost = optimalCost;
    bestSolution.relativeError = calculateRelativeError(bestSolution.cost, optimalCost);
    bestSolution.stopReason = "TimeLimit";
    bestSolution.seed = seed;
    bestSolution.effectiveInitialPheromone = effectiveInitialPheromone;

    std::mt19937 rng(seed);
    std::uniform_int_distribution<int> startCityDistribution(0, instance.dimension - 1);
    const bool symmetricMatrix = isSymmetricDistanceMatrix(instance);

    const auto start = std::chrono::high_resolution_clock::now();

    while (true) {
        if (isTimeLimitReached(start, params.maxTimeSeconds)) {
            bestSolution.stopReason = "TimeLimit";
            break;
        }

        ++bestSolution.iterations;
        std::vector<AntTour> tours;
        tours.reserve(antsCount);

        if (params.depositTiming == PheromoneDepositTiming::AfterMove) {
            evaporatePheromones(pheromones, params.r);
        }

        for (int ant = 0; ant < antsCount; ++ant) {
            const int startCity = startCityDistribution(rng);
            AntTour tour = buildAntTour(instance, pheromones, params, rng, startCity, symmetricMatrix);

            if (tour.cost < bestSolution.cost) {
                bestSolution.cost = tour.cost;
                bestSolution.path = tour.path;
                bestSolution.optimalCost = optimalCost;
                bestSolution.relativeError = calculateRelativeError(bestSolution.cost, optimalCost);
            }

            tours.push_back(std::move(tour));
        }

        if (params.depositTiming == PheromoneDepositTiming::AfterTour) {
            evaporatePheromones(pheromones, params.r);
            for (const AntTour& tour : tours) {
                depositPheromoneOnTour(instance, pheromones, tour, params.depositAmount, params.mode);
            }
        }

        bestSolution.optimalCost = optimalCost;
        bestSolution.relativeError = calculateRelativeError(bestSolution.cost, optimalCost);

        const auto currentTime = std::chrono::high_resolution_clock::now();
        bestSolution.history.push_back({
                                               bestSolution.iterations,
                                               elapsedMilliseconds(start, currentTime),
                                               bestSolution.cost,
                                               bestSolution.relativeError
                                       });

        if (params.stopOnTargetError && bestSolution.relativeError <= params.targetError) {
            bestSolution.stopReason = "TargetErrorReached";
            break;
        }
    }

    const auto end = std::chrono::high_resolution_clock::now();
    bestSolution.timeMs = elapsedMilliseconds(start, end);
    return bestSolution;
}
