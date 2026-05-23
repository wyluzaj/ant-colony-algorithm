#include "config.h"

#include "pheromone_update.h"
#include "utils.h"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <stdexcept>

namespace {
    std::string trim(const std::string& text) {
        size_t first = 0;
        while (first < text.size() && std::isspace(static_cast<unsigned char>(text[first]))) {
            ++first;
        }

        size_t last = text.size();
        while (last > first && std::isspace(static_cast<unsigned char>(text[last - 1]))) {
            --last;
        }

        return text.substr(first, last - first);
    }

    std::string removeInlineComment(const std::string& line) {
        const size_t commentPosition = line.find('#');
        if (commentPosition == std::string::npos) {
            return line;
        }
        return line.substr(0, commentPosition);
    }

    std::string normalizeKey(std::string key) {
        key = trim(key);
        std::transform(key.begin(), key.end(), key.begin(), [](unsigned char c) {
            return static_cast<char>(std::tolower(c));
        });
        return key;
    }

    bool parseBool(const std::string& value) {
        std::string normalized = value;
        std::transform(normalized.begin(), normalized.end(), normalized.begin(), [](unsigned char c) {
            return static_cast<char>(std::tolower(c));
        });

        if (normalized == "true" || normalized == "1" || normalized == "yes" || normalized == "tak") {
            return true;
        }
        if (normalized == "false" || normalized == "0" || normalized == "no" || normalized == "nie") {
            return false;
        }

        throw std::runtime_error("Cannot parse bool value: " + value);
    }

    void setConfigValue(AppConfig& config, const std::string& rawKey, const std::string& rawValue) {
        const std::string key = normalizeKey(rawKey);
        const std::string value = trim(rawValue);

        if (key == "instancepath") {
            config.instancePath = normalizePath(value);
        } else if (key == "outputpath") {
            config.outputPath = normalizePath(value);
        } else if (key == "optimalpath") {
            config.optimalPath = normalizePath(value);
        } else if (key == "resultsdirectory" || key == "resultsdir") {
            config.resultsDirectory = normalizePath(value);
        } else if (key == "ants") {
            config.aco.ants = std::stoi(value);
        } else if (key == "runs") {
            config.aco.runs = std::stoi(value);
        } else if (key == "seed") {
            config.aco.seed = static_cast<unsigned int>(std::stoul(value));
        } else if (key == "maxtimeseconds") {
            config.aco.maxTimeSeconds = std::stod(value);
        } else if (key == "stopontargeterror") {
            config.aco.stopOnTargetError = parseBool(value);
        } else if (key == "targeterror") {
            config.aco.targetError = std::stod(value);
        } else if (key == "alpha") {
            config.aco.alpha = std::stod(value);
        } else if (key == "beta") {
            config.aco.beta = std::stod(value);
        } else if (key == "r" || key == "evaporationrate") {
            config.aco.r = std::stod(value);
        } else if (key == "initialpheromone" || key == "tinitialpheromone") {
            config.aco.initialPheromone = std::stod(value);
        } else if (key == "depositamount") {
            config.aco.depositAmount = std::stod(value);
        } else if (key == "usetwoopt" || key == "twoopt") {
            config.aco.useTwoOpt = parseBool(value);
        } else if (key == "localsearch") {
            const std::string normalizedValue = normalizeKey(value);
            if (normalizedValue == "none" || normalizedValue == "off" || normalizedValue == "false" || normalizedValue == "0") {
                config.aco.useTwoOpt = false;
            } else if (normalizedValue == "twoopt" || normalizedValue == "2opt" || normalizedValue == "2-opt") {
                config.aco.useTwoOpt = true;
            } else {
                throw std::runtime_error("Unknown localSearch value: " + value);
            }
        } else if (key == "deposittiming" || key == "depositetiming") {
            config.aco.depositTiming = pheromoneDepositTimingFromString(value);
        } else if (key == "mode" || key == "updatemode") {
            config.aco.mode = pheromoneUpdateModeFromString(value);
        } else {
            throw std::runtime_error("Unknown config key: " + rawKey);
        }
    }
}

AppConfig readConfig(const std::string& filePath) {
    std::ifstream file(filePath);
    if (!file.is_open()) {
        throw std::runtime_error("Cannot open config file: " + filePath);
    }

    AppConfig config;
    std::string line;
    int lineNumber = 0;

    while (std::getline(file, line)) {
        ++lineNumber;
        line = trim(removeInlineComment(line));
        if (line.empty()) {
            continue;
        }

        size_t separator = line.find('=');
        if (separator == std::string::npos) {
            separator = line.find(':');
        }
        if (separator == std::string::npos) {
            throw std::runtime_error("Invalid config line " + std::to_string(lineNumber) + ": " + line);
        }

        const std::string key = line.substr(0, separator);
        const std::string value = line.substr(separator + 1);
        setConfigValue(config, key, value);
    }

    if (config.instancePath.empty()) {
        throw std::runtime_error("Config must contain instancePath.");
    }
    if (config.outputPath.empty()) {
        throw std::runtime_error("Config must contain outputPath.");
    }
    if (config.aco.ants < 0) {
        throw std::runtime_error("ants must be >= 0. Use ants = 0 to set ants = number of cities.");
    }
    if (config.aco.runs <= 0) {
        throw std::runtime_error("runs must be > 0.");
    }
    if (config.aco.maxTimeSeconds <= 0.0) {
        throw std::runtime_error("maxTimeSeconds must be > 0.");
    }
    if (config.aco.alpha <= 0.0 || config.aco.beta <= 0.0) {
        throw std::runtime_error("alpha and beta must be > 0.");
    }
    if (config.aco.r < 0.0 || config.aco.r > 1.0) {
        throw std::runtime_error("r must be in range [0, 1].");
    }

    validatePheromoneSettings(config.aco.mode, config.aco.depositTiming);
    return config;
}

std::unordered_map<std::string, int> readOptimalValues(const std::string& filePath) {
    std::ifstream file(filePath);
    if (!file.is_open()) {
        throw std::runtime_error("Cannot open optimal values file: " + filePath);
    }

    std::unordered_map<std::string, int> values;
    std::string line;
    int lineNumber = 0;

    while (std::getline(file, line)) {
        ++lineNumber;
        line = trim(removeInlineComment(line));
        if (line.empty()) {
            continue;
        }

        size_t separator = line.find(':');
        if (separator == std::string::npos) {
            separator = line.find('=');
        }
        if (separator == std::string::npos) {
            throw std::runtime_error("Invalid optimal file line " + std::to_string(lineNumber) + ": " + line);
        }

        std::string name = trim(line.substr(0, separator));
        const int cost = std::stoi(trim(line.substr(separator + 1)));
        values[name] = cost;
        values[getFileNameWithoutExtension(name)] = cost;
    }

    return values;
}

int findOptimalCost(
        const std::unordered_map<std::string, int>& optimalValues,
        const std::string& instancePath,
        const std::string& instanceName
) {
    const auto byName = optimalValues.find(instanceName);
    if (byName != optimalValues.end()) {
        return byName->second;
    }

    const std::string fileName = getFileNameWithoutExtension(instancePath);
    const auto byFileName = optimalValues.find(fileName);
    if (byFileName != optimalValues.end()) {
        return byFileName->second;
    }

    return 0;
}
