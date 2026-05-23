#ifndef PEA4_TSP_INSTANCE_H
#define PEA4_TSP_INSTANCE_H

#include <cstddef>
#include <string>
#include <vector>

struct City {
    int id = 0;
    double x = 0.0;
    double y = 0.0;
};

struct TSPInstance {
    bool symmetric = true;
    std::string name;
    std::string type;
    std::string edge_weight_type;
    std::string edge_weight_format;
    std::string display_data_type;
    int dimension = 0;

    std::vector<City> cities;

    // Flat distance matrix: distance(i, j) is stored at i * dimension + j.
    // It is faster for ants than vector<vector<int>>, because data is continuous in memory.
    std::vector<int> distanceMatrix;

    std::vector<std::vector<int>> sortedNeighbors;
    std::vector<std::vector<int>> sortedInNeighbors;

    [[nodiscard]] std::size_t index(int row, int column) const {
        return static_cast<std::size_t>(row) * static_cast<std::size_t>(dimension) +
               static_cast<std::size_t>(column);
    }

    [[nodiscard]] int distance(int from, int to) const {
        return distanceMatrix[index(from, to)];
    }

    int& distanceRef(int from, int to) {
        return distanceMatrix[index(from, to)];
    }

    [[nodiscard]] bool isValid() const {
        return dimension > 0 &&
               distanceMatrix.size() == static_cast<std::size_t>(dimension) * static_cast<std::size_t>(dimension);
    }
};

#endif // PEA4_TSP_INSTANCE_H
