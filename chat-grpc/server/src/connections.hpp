#pragma once

#include <atomic>
#include <condition_variable>
#include <functional>
#include <memory>
#include <mutex>
#include <queue>
#include <thread>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "database.hpp"
#include "protocols/utils.hpp"

class Connection : public std::enable_shared_from_this<Connection> {
    static constexpr size_t BUF_SIZE = 4096;

    std::queue<Data> requests_;
    std::mutex requests_mutex_;

    std::array<uint8_t, BUF_SIZE> send_buf_;
    std::array<uint8_t, BUF_SIZE> recv_buf_;

    std::function<void(Id, std::shared_ptr<Connection>)> assign_connection_;
    std::function<void(Id, std::shared_ptr<Connection>)> unassign_connection_;

    std::optional<Id> user_id_;

   public:
    void send(const std::vector<uint8_t> &data);
    Data recv();

   public:
    int sock;

    std::function<void(Id, const Data &)> broadcast;

    Connection(
        int sock,
        std::function<void(Id, std::shared_ptr<Connection>)> assign_connection,
        std::function<void(Id, std::shared_ptr<Connection>)>
            unassign_connection,
        std::function<void(Id, const Data &)> broadcast);
    ~Connection();

    void handle(Db &db);
    void request(const std::vector<uint8_t> &data);
};

class WorkerPool {
    static constexpr size_t MIN_WORKERS = 4;
    static constexpr size_t MAX_WORKERS = 16;

    std::vector<std::thread> workers_;
    std::condition_variable workers_cv_;
    std::atomic<bool> workers_stop_;

    std::queue<std::shared_ptr<Connection>> connections_;
    std::mutex connections_mutex_;

    std::unordered_map<Id, std::unordered_set<std::shared_ptr<Connection>>>
        users_to_connections_;
    std::mutex users_to_connections_mutex_;

    void start_worker();
    void adjust_workers();
    void assign_connection(Id user_id, std::shared_ptr<Connection> conn);
    void unassign_connection(Id user_id, std::shared_ptr<Connection> conn);
    void broadcast(Id user_id, const Data &data);

   public:
    WorkerPool();
    ~WorkerPool();
    void handle(int sock);
};
