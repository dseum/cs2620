#pragma once

#include <fstream>
#include <vector>

struct Message {
    int senderRank;
    int logicalTimestamp;
};

void runNode(int rank, std::ofstream &logFile,
             const std::vector<int> &socketFds);
