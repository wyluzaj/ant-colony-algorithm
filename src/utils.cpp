#include "utils.h"

#include <algorithm>
#include <cmath>
#include <cctype>
#include <limits>
#include <sstream>
#include <stdexcept>

namespace {
    constexpr double PI = 3.14159265358979323846;
    constexpr double EARTH_RADIUS = 6378.388;

    double toGeoRadians(double coordinate) {
        const int deg = static_cast<int>(coordinate);
        const double min = coordinate - deg;
        return PI * (deg + 5.0 * min / 3.0) / 180.0;
    }
}

int calculateEuclideanDistance(const City& a, const City& b) {
    const double dx = a.x - b.x;
    const double dy = a.y - b.y;
    return static_cast<int>(std::round(std::sqrt(dx * dx + dy * dy)));
}

int calculateGeoDistance(const City& a, const City& b) {
    const double latA = toGeoRadians(a.x);
    const double lonA = toGeoRadians(a.y);
    const double latB = toGeoRadians(b.x);
    const double lonB = toGeoRadians(b.y);

    const double q1 = std::cos(lonA - lonB);
    const double q2 = std::cos(latA - latB);
    const double q3 = std::cos(latA + latB);

    double argument = 0.5 * ((1.0 + q1) * q2 - (1.0 - q1) * q3);
    argument = std::max(-1.0, std::min(1.0, argument));

    return static_cast<int>(EARTH_RADIUS * std::acos(argument) + 1.0);
}

void buildDistanceMatrixFromCoordinates(TSPInstance& instance) {
    const int n = instance.dimension;
    if (n <= 0) {
        throw std::runtime_error("Invalid instance size");
    }
    if (static_cast<int>(instance.cities.size()) != n) {
        throw std::runtime_error("The number of cities does not match the DIMENSION.");
    }

    instance.distanceMatrix.assign(static_cast<size_t>(n) * static_cast<size_t>(n), 0);

    for (int i = 0; i < n; ++i) {
        for (int j = 0; j < n; ++j) {
            if (i == j) {
                instance.distanceRef(i, j) = 0;
            } else if (instance.edge_weight_type == "GEO") {
                instance.distanceRef(i, j) = calculateGeoDistance(instance.cities[i], instance.cities[j]);
            } else if (instance.edge_weight_type == "EUC_2D" || instance.edge_weight_type.empty()) {
                instance.distanceRef(i, j) = calculateEuclideanDistance(instance.cities[i], instance.cities[j]);
            } else {
                throw std::runtime_error("Unsupported EDGE_WEIGHT_TYPE for coordinates:" + instance.edge_weight_type);
            }
        }
    }

    instance.symmetric = true;
    if (instance.type.empty()) {
        instance.type = "TSP";
    }
    if (instance.edge_weight_type.empty()) {
        instance.edge_weight_type = "EUC_2D";
    }
    buildSortedNeighbors(instance);
}

void buildSortedNeighbors(TSPInstance& instance) {
    const int n = instance.dimension;
    instance.sortedNeighbors.assign(n, {});
    instance.sortedInNeighbors.assign(n, {});

    for (int i = 0; i < n; ++i) {
        for (int j = 0; j < n; ++j) {
            if (i != j) {
                instance.sortedNeighbors[i].push_back(j);
                instance.sortedInNeighbors[i].push_back(j);
            }
        }

        std::sort(instance.sortedNeighbors[i].begin(), instance.sortedNeighbors[i].end(),
                  [&](int a, int b) { return instance.distance(i, a) < instance.distance(i, b); });

        std::sort(instance.sortedInNeighbors[i].begin(), instance.sortedInNeighbors[i].end(),
                  [&](int a, int b) { return instance.distance(a, i) < instance.distance(b, i); });
    }
}

int calculatePartialPathCost(const TSPInstance& instance, const std::vector<int>& path) {
    if (instance.distanceMatrix.empty()) {
        throw std::runtime_error("The distance matrix has not been constructed.");
    }
    if (path.size() < 2) {
        return 0;
    }

    int totalCost = 0;
    for (size_t i = 0; i + 1 < path.size(); ++i) {
        const int from = path[i];
        const int to = path[i + 1];
        if (from < 0 || from >= instance.dimension || to < 0 || to >= instance.dimension) {
            throw std::runtime_error("The path contains an invalid city index.");
        }
        totalCost += instance.distance(from, to);
    }
    return totalCost;
}

int calculatePathCost(const TSPInstance& instance, const std::vector<int>& path) {
    if (instance.distanceMatrix.empty()) {
        throw std::runtime_error("The distance matrix has not been constructed.");
    }
    if (path.empty()) {
        return 0;
    }
    if (path.size() != static_cast<size_t>(instance.dimension)) {
        throw std::runtime_error("The path must include each city exactly once.");
    }

    int totalCost = calculatePartialPathCost(instance, path);
    totalCost += instance.distance(path.back(), path.front());
    return totalCost;
}

std::string getFileNameWithoutExtension(const std::string& filePath) {
    std::string name = filePath;
    const size_t slash = name.find_last_of("/\\");
    if (slash != std::string::npos) {
        name = name.substr(slash + 1);
    }
    const size_t dot = name.find_last_of('.');
    if (dot != std::string::npos) {
        name = name.substr(0, dot);
    }
    return name;
}

std::string normalizePath(const std::string& path) {
    std::string result = path;
    while (!result.empty() && std::isspace(static_cast<unsigned char>(result.front()))) {
        result.erase(result.begin());
    }
    while (!result.empty() && std::isspace(static_cast<unsigned char>(result.back()))) {
        result.pop_back();
    }
    if (result.size() >= 2 && result.front() == '"' && result.back() == '"') {
        result = result.substr(1, result.size() - 2);
    }
    return result;
}

double elapsedMilliseconds(
        const std::chrono::high_resolution_clock::time_point& start,
        const std::chrono::high_resolution_clock::time_point& end
) {
    return std::chrono::duration<double, std::milli>(end - start).count();
}

std::string pathToString(const std::vector<int>& path, const TSPInstance& instance) {
    std::ostringstream oss;
    const bool hasCityIds = instance.cities.size() == static_cast<size_t>(instance.dimension);

    for (size_t i = 0; i < path.size(); ++i) {
        if (i > 0) {
            oss << " -> ";
        }
        const int idx = path[i];
        if (hasCityIds && idx >= 0 && idx < instance.dimension) {
            oss << instance.cities[idx].id;
        } else {
            oss << idx + 1;
        }
    }

    if (!path.empty()) {
        const int first = path.front();
        if (hasCityIds && first >= 0 && first < instance.dimension) {
            oss << " -> " << instance.cities[first].id;
        } else {
            oss << " -> " << first + 1;
        }
    }
    return oss.str();
}
