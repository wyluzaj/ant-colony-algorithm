#ifndef PEA4_UTILS_H
#define PEA4_UTILS_H
#include <chrono>
#include <string>
#include <vector>

#include "tsp_instance.h"

int calculateEuclideanDistance(const City& a, const City& b);
int calculateGeoDistance(const City& a, const City& b);
void buildDistanceMatrixFromCoordinates(TSPInstance& instance);
void buildSortedNeighbors(TSPInstance& instance);

int calculatePathCost(const TSPInstance& instance, const std::vector<int>& path);
int calculatePartialPathCost(const TSPInstance& instance, const std::vector<int>& path);

std::string getFileNameWithoutExtension(const std::string& filePath);
std::string normalizePath(const std::string& path);
std::string pathToString(const std::vector<int>& path, const TSPInstance& instance);

double elapsedMilliseconds(
        const std::chrono::high_resolution_clock::time_point& start,
        const std::chrono::high_resolution_clock::time_point& end
);
#endif //PEA4_UTILS_H
