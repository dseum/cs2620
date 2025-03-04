#pragma once

#include <vector>

#include "logger.hpp"

struct Message {
    int senderRank;
    int logicalTimestamp;
};

void runNode(int rank, Logger &logger, const std::vector<int> &channels, int num_vms);
