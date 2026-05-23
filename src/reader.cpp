#include "reader.h"

#include "utils.h"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {
    std::string trim(const std::string& s) {
        const size_t first = s.find_first_not_of(" \t\r\n");
        if (first == std::string::npos) {
            return "";
        }
        const size_t last = s.find_last_not_of(" \t\r\n");
        return s.substr(first, last - first + 1);
    }

    bool startsWith(const std::string& text, const std::string& prefix) {
        return text.rfind(prefix, 0) == 0;
    }

    std::string valueAfterColon(const std::string& line) {
        const size_t pos = line.find(':');
        if (pos == std::string::npos) {
            return "";
        }
        return trim(line.substr(pos + 1));
    }

    void validateBasicHeader(const TSPInstance& instance, const std::string& filePath) {
        if (instance.dimension <= 0) {
            throw std::runtime_error("Missing or incorrect DIMENSION in file: " + filePath);
        }
        if (instance.type.empty()) {
            throw std::runtime_error("Missing TYPE in file: " + filePath);
        }
        if (instance.type != "TSP" && instance.type != "ATSP") {
            throw std::runtime_error("Unknown TYPE in file: " + instance.type);
        }
    }

    int readWeight(std::istream& file, const std::string& format) {
        int weight = 0;
        if (!(file >> weight)) {
            throw std::runtime_error("Not enough data in EDGE_WEIGHT_SECTION for format " + format + ".");
        }
        return weight;
    }

    void readFullMatrix(std::istream& file, TSPInstance& instance) {
        const int n = instance.dimension;
        for (int i = 0; i < n; ++i) {
            for (int j = 0; j < n; ++j) {
                instance.distanceRef(i, j) = readWeight(file, "FULL_MATRIX");
            }
        }
    }

    void readLowerDiagRow(std::istream& file, TSPInstance& instance) {
        const int n = instance.dimension;
        for (int i = 0; i < n; ++i) {
            for (int j = 0; j <= i; ++j) {
                const int weight = readWeight(file, "LOWER_DIAG_ROW");
                instance.distanceRef(i, j) = weight;
                instance.distanceRef(j, i) = weight;
            }
        }
        instance.symmetric = true;
    }

    void readUpperDiagRow(std::istream& file, TSPInstance& instance) {
        const int n = instance.dimension;
        for (int i = 0; i < n; ++i) {
            for (int j = i; j < n; ++j) {
                const int weight = readWeight(file, "UPPER_DIAG_ROW");
                instance.distanceRef(i, j) = weight;
                instance.distanceRef(j, i) = weight;
            }
        }
        instance.symmetric = true;
    }

    void validateExplicitFormat(const TSPInstance& instance, const std::string& filePath) {
        if (instance.edge_weight_type != "EXPLICIT") {
            throw std::runtime_error("EDGE_WEIGHT_SECTION needed EDGE_WEIGHT_TYPE = EXPLICIT. File: " + filePath);
        }
        if (instance.edge_weight_format != "FULL_MATRIX" &&
            instance.edge_weight_format != "LOWER_DIAG_ROW" &&
            instance.edge_weight_format != "UPPER_DIAG_ROW") {
            throw std::runtime_error(
                    "Unsupported EDGE_WEIGHT_FORMAT: " + instance.edge_weight_format +
                    "Supported: FULL_MATRIX, LOWER_DIAG_ROW, UPPER_DIAG_ROW. File: " + filePath
            );
        }
    }
}

TSPInstance readTSPInstance(const std::string& filePath) {
    std::ifstream file(filePath);
    if (!file.is_open()) {
        throw std::runtime_error("Cannot open file: " + filePath);
    }

    TSPInstance instance;
    std::string line;
    bool inNodeCoordSection = false;
    bool inEdgeWeightSection = false;

    while (std::getline(file, line)) {
        line = trim(line);
        if (line.empty()) {
            continue;
        }
        if (line == "EOF") {
            break;
        }

        if (line == "NODE_COORD_SECTION") {
            validateBasicHeader(instance, filePath);
            if (instance.edge_weight_type != "EUC_2D" && instance.edge_weight_type != "GEO") {
                throw std::runtime_error(
                        "NODE_COORD_SECTION only supports EDGE_WEIGHT_TYPE = EUC_2D or GEO. Loaded: " +
                        instance.edge_weight_type
                );
            }
            inNodeCoordSection = true;
            continue;
        }

        if (line == "EDGE_WEIGHT_SECTION") {
            validateBasicHeader(instance, filePath);
            validateExplicitFormat(instance, filePath);
            inEdgeWeightSection = true;
            break;
        }

        if (!inNodeCoordSection) {
            if (startsWith(line, "NAME")) {
                instance.name = valueAfterColon(line);
            } else if (startsWith(line, "TYPE")) {
                instance.type = valueAfterColon(line);
                instance.symmetric = instance.type != "ATSP";
            } else if (startsWith(line, "DIMENSION")) {
                instance.dimension = std::stoi(valueAfterColon(line));
            } else if (startsWith(line, "EDGE_WEIGHT_TYPE")) {
                instance.edge_weight_type = valueAfterColon(line);
            } else if (startsWith(line, "EDGE_WEIGHT_FORMAT")) {
                instance.edge_weight_format = valueAfterColon(line);
            } else if (startsWith(line, "DISPLAY_DATA_TYPE")) {
                instance.display_data_type = valueAfterColon(line);
            }
        } else {
            std::istringstream iss(line);
            City city{};
            if (!(iss >> city.id >> city.x >> city.y)) {
                throw std::runtime_error("Error reading city in file: " + filePath + "Line:" + line);
            }
            instance.cities.push_back(city);
        }
    }

    validateBasicHeader(instance, filePath);

    if (inNodeCoordSection) {
        if (static_cast<int>(instance.cities.size()) != instance.dimension) {
            throw std::runtime_error("The number of cities does not match the DIMENSION in the file: " + filePath);
        }
        buildDistanceMatrixFromCoordinates(instance);
        return instance;
    }

    if (inEdgeWeightSection) {
        instance.distanceMatrix.assign(static_cast<size_t>(instance.dimension) * static_cast<size_t>(instance.dimension), 0);

        if (instance.edge_weight_format == "FULL_MATRIX") {
            readFullMatrix(file, instance);
        } else if (instance.edge_weight_format == "LOWER_DIAG_ROW") {
            readLowerDiagRow(file, instance);
        } else if (instance.edge_weight_format == "UPPER_DIAG_ROW") {
            readUpperDiagRow(file, instance);
        }

        buildSortedNeighbors(instance);
        return instance;
    }

    throw std::runtime_error("Not found: NODE_COORD_SECTION or EDGE_WEIGHT_SECTION in file: " + filePath);
}
