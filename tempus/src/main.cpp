#include <arpa/inet.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include <iostream>
#include <numeric>
#include <print>
#include <vector>

#include "logger.hpp"
#include "logic.hpp"

int main() {
    srand(1);

    constexpr int num_vms = 3;  // number of vms to simulate; should not change
    constexpr int min_port = 17050;  // minimum port number to use

    std::vector<int> ports(num_vms);
    std::iota(ports.begin(), ports.end(), min_port);

    for (int rank = 0; rank < num_vms; ++rank) {
        std::println("Starting rank {} on port {}", rank, ports[rank]);
        pid_t pid = fork();
        if (pid < 0) {  // No process
            std::cerr << "Failed to fork process" << std::endl;
            return 1;
        } else if (pid == 0) {  // Child process
            Logger logger(rank);

            // Initializes channels
            std::vector<int> channels(num_vms - 1);
            {
                int sock;
                if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
                    logger.write("[ERROR]: Failed to create socket: {}",
                                 strerror(errno));
                    return 1;
                }

                int opt = 1;
                setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

                struct sockaddr_in addr;
                addr.sin_family = AF_INET;
                addr.sin_port = htons(ports[rank]);
                addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);

                logger.write("[INFO]: Binding to port {}", ports[rank]);
                if (bind(sock, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
                    close(sock);
                    logger.write("[ERROR]: Failed to bind socket: {}",
                                 strerror(errno));
                    return 1;
                }

                logger.write("[INFO]: Listening on port {}", ports[rank]);
                if (listen(sock, num_vms - 1) < 0) {
                    close(sock);
                    logger.write("[ERROR]: Failed to listen on socket: {}",
                                 strerror(errno));
                    return 1;
                }

                // Accepts
                for (int i = 0; i < rank; i++) {
                    int other_sock;
                    logger.write("[INFO]: Waiting for connect");
                    if ((other_sock = accept(sock, nullptr, nullptr)) < 0) {
                        for (int channel : channels) {
                            close(channel);
                        }
                        close(sock);
                        logger.write("[ERROR]: Failed to accept on socket: {}",
                                     strerror(errno));
                        return 1;
                    }
                    char buf[sizeof(int)];
                    size_t size = 0;
                    while (size != sizeof(buf)) {
                        int bytes_read = recv(other_sock, &buf + size,
                                              sizeof(buf) - size, 0);
                        if (bytes_read <= 0) {
                            for (int channel : channels) {
                                close(channel);
                            }
                            close(other_sock);
                            close(sock);
                            logger.write(
                                "[ERROR]: Failed to read rank from other "
                                "channel: {}",
                                strerror(errno));
                            return 1;
                        }
                        size += bytes_read;
                    }
                    int other_rank =
                        *reinterpret_cast<int *>(buf);  // Is < rank
                    logger.write("[INFO]: Accepted channel from rank {}",
                                 other_rank);
                    channels[other_rank] = other_sock;
                }

                // Connects
                for (int other_rank = rank + 1; other_rank < num_vms;
                     ++other_rank) {
                    int i = other_rank - 1;
                    if ((channels[i] = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
                        for (int channel : channels) {
                            close(channel);
                        }
                        close(sock);
                        logger.write("[ERROR]: Failed to create channel: {}",
                                     strerror(errno));
                        return 1;
                    }

                    struct sockaddr_in other_addr;
                    other_addr.sin_family = AF_INET;
                    other_addr.sin_port = htons(ports[other_rank]);
                    other_addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);

                    while (true) {
                        if (connect(channels[i], (struct sockaddr *)&other_addr,
                                    sizeof(other_addr)) < 0) {
                            logger.write(
                                "[ERROR]: Failed to connect to rank {}: {}",
                                other_rank, strerror(errno));
                            close(channels[i]);
                            if ((channels[i] =
                                     socket(AF_INET, SOCK_STREAM, 0)) < 0) {
                                for (int channel : channels) {
                                    close(channel);
                                }
                                close(sock);
                                logger.write(
                                    "[ERROR]: Failed to create channel: {}",
                                    strerror(errno));
                                return 1;
                            }
                            sleep(1);
                            continue;
                        }
                        break;
                    }

                    logger.write("[INFO]: Connected to rank {}", other_rank);

                    char buf[sizeof(int)];
                    std::memcpy(buf, &rank, sizeof(rank));
                    size_t size = 0;
                    while (size != sizeof(rank)) {
                        int bytes_sent = send(channels[i], &buf + size,
                                              sizeof(buf) - size, 0);
                        if (bytes_sent <= 0) {
                            for (int channel : channels) {
                                close(channel);
                            }
                            close(sock);
                            logger.write(
                                "[ERROR]: Failed to send rank to other "
                                "channel: {}",
                                strerror(errno));
                            return 1;
                        }
                        size += bytes_sent;
                    }
                }
                close(sock);
            }
            try {
                runNode(rank, logger, channels);
            } catch (const std::exception &e) {
                logger.write("[ERROR]: {}", e.what());
            }
            for (int channel : channels) {
                close(channel);
            }
            break;
        }
    }

    while (wait(NULL) > 0);  // Waits for all children to finish

    return 0;
}
