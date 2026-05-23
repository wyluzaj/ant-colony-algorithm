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
                if (!visited[candidate] && instance.distanceMatrix[current][candidate] < bestDistance) {
                    bestDistance = instance.distanceMatrix[current][candidate];
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

    std::vector<std::vector<double>> initializePheromones(
            const TSPInstance& instance,
            double effectiveInitialPheromone
    ) {
        std::vector<std::vector<double>> pheromones(
                instance.dimension,
                std::vector<double>(instance.dimension, effectiveInitialPheromone)
        );

        for (int i = 0; i < instance.dimension; ++i) {
            pheromones[i][i] = 0.0;
        }

        return pheromones;
    }

    int chooseNextCity(
            const TSPInstance& instance,
            const std::vector<std::vector<double>>& pheromones,
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

            const int distance = instance.distanceMatrix[currentCity][city];
            if (distance <= 0) {
                continue;
            }

            const double tau = std::max(pheromones[currentCity][city], 1e-12);
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

    AntTour buildAntTour(
            const TSPInstance& instance,
            std::vector<std::vector<double>>& pheromones,
            const ACOParameters& params,
            std::mt19937& rng,
            int startCity
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

        tour.cost = calculatePathCost(instance, tour.path);
        return tour;
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
    std::vector<std::vector<double>> pheromones = initializePheromones(
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
            AntTour tour = buildAntTour(instance, pheromones, params, rng, startCity);

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
