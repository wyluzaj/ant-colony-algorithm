#ifndef PEA4_TSP_INSTANCE_H
#define PEA4_TSP_INSTANCE_H
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
    std::vector<std::vector<int>> distanceMatrix;
    std::vector<std::vector<int>> sortedNeighbors;
    std::vector<std::vector<int>> sortedInNeighbors;

    [[nodiscard]] bool isValid() const {
        return dimension > 0 &&
               static_cast<int>(distanceMatrix.size()) == dimension;
    }
};

#endif //PEA4_TSP_INSTANCE_H
