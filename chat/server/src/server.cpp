#include "server.hpp"

#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include <algorithm>
#include <csignal>
#include <cstring>
#include <format>
#include <iostream>
#include <mutex>
#include <print>

Server::Server(std::string host, int port) {
    // Creates IPv4 TCP socket
    int server_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (server_sock == -1) {
        throw std::runtime_error(
            std::format("Socket creation failed: {}", std::strerror(errno)));
    }

    // Sets up the server address structure
    sockaddr_in server_addr;
    std::memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    server_addr.sin_addr.s_addr =
        inet_addr(host.c_str());  // Uses network byte order

    // Binds the socket
    if (bind(server_sock, reinterpret_cast<sockaddr*>(&server_addr),
             sizeof(server_addr)) == -1) {
        close(server_sock);
        throw std::runtime_error(
            std::format("bind failed: {}", std::strerror(errno)));
    }

    // Listens for connections
    if (listen(server_sock, SOMAXCONN) == -1) {
        close(server_sock);
        throw std::runtime_error(
            std::format("listen failed: {}", std::strerror(errno)));
    }

    std::println("listening on {}:{}", host, port);

    while (true) {
        sockaddr_in client_addr;
        socklen_t client_size = sizeof(client_addr);
        int client_sock =
            accept(server_sock, reinterpret_cast<sockaddr*>(&client_addr),
                   &client_size);
        if (client_sock == -1) {
            std::println(std::cerr, "accept failed: {}", std::strerror(errno));
            continue;
        }

        std::println("accepted connection on {}", client_sock);
        worker_pool_.handle(client_sock);
    }
    close(server_sock);
}
