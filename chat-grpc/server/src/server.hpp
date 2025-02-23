#pragma once

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sqlite3.h>
#include <sys/socket.h>

#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

#include "connections.hpp"

class Server {
    WorkerPool worker_pool_;

   public:
    Server(std::string address, int port);
};
