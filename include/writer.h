#ifndef PEA4_WRITER_H
#define PEA4_WRITER_H

#include "ant_colony.h"

#include <string>
#include <vector>

struct AlgorithmResult {
    std::string instanceName;
    std::string algorithmName;
    int dimension = 0;

    int runNumber = 0;
    unsigned int seed = 0;

    int bestCost = 0;
    int optimalCost = 0;
    double relativeError = -1.0;
    double timeMs = 0.0;
    int iterations = 0;
    std::string stopReason;

    int ants = 0;
    int runs = 1;
    double maxTimeSeconds = 0.0;
    bool stopOnTargetError = false;
    double targetError = 0.0;

    double alpha = 0.0;
    double beta = 0.0;
    double r = 0.0;
    double initialPheromone = 0.0;
    double effectiveInitialPheromone = 0.0;
    double depositAmount = 0.0;
    bool useTwoOpt = false;

    std::string mode;

    std::vector<int> bestPath;
};

void writeResultCsvHeaderIfNeeded(const std::string& filePath);
void appendResultToCsv(const std::string& filePath, const AlgorithmResult& result);
void appendAggregateToCsv(const std::string& filePath, const std::vector<AlgorithmResult>& results);
void writeHistoryToCsv(const std::string& filePath, const std::vector<ACOHistoryEntry>& history);

#endif // PEA4_WRITER_H
