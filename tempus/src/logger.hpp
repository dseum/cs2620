#pragma once

#include <format>
#include <fstream>
#include <mutex>

class Logger {
    std::ofstream file;
    std::mutex file_mutex;

   public:
    Logger(int rank);

    template <typename... Args>
    void write(std::format_string<Args...> fmt, Args &&...args) {
        std::unique_lock<std::mutex> lock(file_mutex);
        file << std::format(fmt, std::forward<Args>(args)...) << "\n";
    }
};
