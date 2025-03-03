#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include <fstream>
#include <numeric>
#include <print>
#include <vector>

#include "logic.hpp"

int main(int argc, const char *argv[]) {
    srand(1);

    constexpr int num_vms = 3;  // number of vms to simulate; should not change
    constexpr int max_ticks =
        6;  // maximum number of ticks a vm can be initialized with
    constexpr int min_prob = 4;      // minimum bound to use for computing
                                     // probability when selecting from [1, 10]
    constexpr int min_port = 14400;  // minimum port number to use

    std::vector<int> ports(num_vms);
    std::iota(ports.begin(), ports.end(), min_port);
    for (int i = 0; i < num_vms; ++i) {
        std::println("VM {}: port {}", i, ports[i]);
    }

    for (int rank = 0; rank < num_vms; ++rank) {
        pid_t pid = fork();
        if (pid < 0) {  // No process
            throw std::runtime_error("Failed to fork");
        } else if (pid == 0) {  // Child process
            std::ofstream logFile(std::format("node_{}.log", rank));
            logFile << std::format("[INIT]: Rank={}", rank) << std::endl;

            // Initializes channels
            std::vector<int> channels(num_vms);
            {
                int sock;
                if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
                    throw std::runtime_error("Failed to create socket");
                }

                struct sockaddr_in addr;
                addr.sin_family = AF_INET;
                addr.sin_port = htons(ports[rank]);
                addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);

                if (bind(sock, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
                    throw std::runtime_error(
                        std::format("Failed to bind socket to port {} err: {}",
                                    ports[rank], strerror(errno)));
                }

                if (listen(sock, num_vms - 1) < 0) {
                    throw std::runtime_error("Failed to listen on socket");
                }

                logFile << std::format("[INFO]: Accept") << std::endl;
                for (int other_rank = 0; other_rank < rank; ++other_rank) {
                    if ((channels[other_rank] =
                             accept(sock, nullptr, nullptr))) {
                        throw std::runtime_error("Failed to accept channel");
                    }
                }

                logFile << std::format("[INFO]: Connect") << std::endl;
                for (int other_rank = rank + 1; other_rank < num_vms;
                     ++other_rank) {
                    if ((channels[other_rank] =
                             socket(AF_INET, SOCK_STREAM, 0) < 0)) {
                        throw std::runtime_error("Failed to create channel");
                    }

                    struct sockaddr_in other_addr;
                    other_addr.sin_family = AF_INET;
                    other_addr.sin_port = htons(ports[other_rank]);
                    other_addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);

                    logFile
                        << std::format("[INFO]: Connecting to {}", other_rank)
                        << std::endl;

                    while (connect(channels[other_rank],
                                   (struct sockaddr *)&other_addr,
                                   sizeof(other_addr)) <
                           0) {  // Retries until connection is established
                        usleep(1000);  // Sleeps briefly to avoid busy waiting
                        logFile << std::format("[INFO]: Retrying") << std::endl;
                    }
                }

                logFile << std::format("[INFO]: Connected") << std::endl;
                close(sock);
            }
            runNode(rank, logFile, channels);
            for (int other_rank = 0; other_rank < num_vms; ++other_rank) {
                if (other_rank == rank) continue;
                close(channels[other_rank]);
            }
        }
    }

    return 0;
}
