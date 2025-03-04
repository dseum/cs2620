#include "logger.hpp"

#include <format>

Logger::Logger(int rank) : file(std::format("rank_{}.log", rank)) {
    std::unique_lock<std::mutex> lock(file_mutex);
    file << std::format("[INIT]: Rank={}", rank) << std::endl;
}
