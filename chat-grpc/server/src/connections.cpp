#include "connections.hpp"

#include <arpa/inet.h>
#include <unistd.h>

#include <algorithm>
#include <array>
#include <print>

#include "database.hpp"
#include "operations.hpp"
#include "protocols/utils.hpp"

void Connection::send(const Data& data) {
    size_t data_sent_size = 0;
    std::println("sending data {}", data);
    while (data_sent_size < data.size()) {
        size_t send_buf_size = std::min(data.size() - data_sent_size, BUF_SIZE);
        std::copy(data.begin() + data_sent_size,
                  data.begin() + data_sent_size + send_buf_size,
                  send_buf_.begin());
        ssize_t sent_size =
            ::send(sock, send_buf_.data(), send_buf_size, MSG_NOSIGNAL);
        if (sent_size <= 0) {
            throw std::runtime_error("disconnected");
        }
        data_sent_size += sent_size;
    }
}

Data Connection::recv() {
    size_t expected_size;
    size_t data_recv_size = 0;

    // recv_buf_ must be greater than or equal to SizeWrapper::size()
    while (data_recv_size < SizeWrapper::size()) {
        ssize_t recv_size = ::recv(sock, recv_buf_.data() + data_recv_size,
                                   SizeWrapper::size() - data_recv_size, 0);
        if (recv_size <= 0) {
            throw std::runtime_error("disconnected");
        }
        data_recv_size += recv_size;
    }
    {
        Data size_data(SizeWrapper::size());
        std::copy(recv_buf_.begin(), recv_buf_.begin() + SizeWrapper::size(),
                  size_data.begin());
        expected_size = SizeWrapper::deserialize_from(size_data, 0).first;
    }

    data_recv_size = 0;
    Data data;
    data.reserve(expected_size);
    while (data_recv_size < expected_size) {
        ssize_t recv_size = ::recv(sock, recv_buf_.data(), BUF_SIZE, 0);
        if (recv_size <= 0) {
            throw std::runtime_error("disconnected");
        }
        data.insert(data.end(), recv_buf_.begin(),
                    recv_buf_.begin() + recv_size);
        data_recv_size += recv_size;
    }

    return data;
}

Connection::Connection(
    int sock,
    std::function<void(Id, std::shared_ptr<Connection>)> assign_connection,
    std::function<void(Id, std::shared_ptr<Connection>)> unassign_connection,
    std::function<void(Id, const Data&)> broadcast)
    : assign_connection_(assign_connection),
      unassign_connection_(unassign_connection),
      sock(sock),
      broadcast(broadcast) {}

Connection::~Connection() {
    close(sock);
    std::println("closed connection on {}", sock);
}

void Connection::handle(Db& db) {
    while (true) {
        {
            std::scoped_lock<std::mutex> lock(requests_mutex_);
            while (!requests_.empty()) {
                Data req = std::move(requests_.front());
                requests_.pop();
                send(req);
            }
        }
        try {
            Data req = recv();
            std::unique_ptr<ClientResponse> res =
                handle_client_request(*this, db, req);
            if (res->operation_code == OperationCode::CLIENT_SIGNIN_USER &&
                res->status_code == StatusCode::SUCCESS) {
                auto siu_res =
                    dynamic_cast<ClientSigninUserResponse*>(res.get());
                user_id_ = siu_res->user_id;
                assign_connection_(user_id_.value(), shared_from_this());
            }
            auto t = res->serialize();
            std::println("sending response of size {}", t.size());
            send(t);
        } catch (const std::exception& e) {
            if (user_id_.has_value()) {
                unassign_connection_(user_id_.value(), shared_from_this());
            }
            return;
        }
    }
}

void Connection::request(const Data& data) {
    std::scoped_lock<std::mutex> lock(requests_mutex_);
    requests_.emplace(data);
}

void WorkerPool::start_worker() {
    Db db;
    while (!workers_stop_) {
        std::shared_ptr<Connection> conn;
        {
            std::unique_lock<std::mutex> lock(connections_mutex_);
            workers_cv_.wait(lock, [this] {
                return workers_stop_ || !connections_.empty();
            });
            if (workers_stop_ && connections_.empty()) {
                return;
            }
            conn = std::move(connections_.front());
            connections_.pop();
        }
        conn->handle(db);
    }
}

void WorkerPool::adjust_workers() {
    std::scoped_lock<std::mutex> lock(connections_mutex_);
    size_t num_workers = workers_.size();
    size_t num_waiting_conns = connections_.size();
    if (num_waiting_conns > num_workers && num_workers < MAX_WORKERS) {
        workers_.emplace_back([this] { start_worker(); });
    } else if (num_waiting_conns < num_workers && num_workers > MIN_WORKERS) {
        workers_cv_.notify_one();
        auto it = std::find_if(workers_.begin(), workers_.end(),
                               [](std::thread& t) { return t.joinable(); });
        if (it != workers_.end()) {
            it->join();
            workers_.erase(it);
        }
    }
}

void WorkerPool::assign_connection(Id user_id,
                                   std::shared_ptr<Connection> conn) {
    std::scoped_lock<std::mutex> lock(users_to_connections_mutex_);
    users_to_connections_[user_id].emplace(conn);
    std::println("assigned connection on {} to user {}", conn->sock, user_id);
}

void WorkerPool::unassign_connection(Id user_id,
                                     std::shared_ptr<Connection> conn) {
    std::scoped_lock<std::mutex> lock(users_to_connections_mutex_);
    if (users_to_connections_[user_id].size() == 1) {
        users_to_connections_.erase(user_id);
        std::println("unassigned connection on {} from user {} and destroyed",
                     conn->sock, user_id);
    } else {
        users_to_connections_[user_id].erase(conn);
        std::println("unassigned connection on {} from user {}", conn->sock,
                     user_id);
    }
}

void WorkerPool::broadcast(Id user_id, const std::vector<uint8_t>& data) {
    std::scoped_lock<std::mutex> lock(users_to_connections_mutex_);
    for (const auto& conn : users_to_connections_[user_id]) {
        conn->request(data);
    }
}

WorkerPool::WorkerPool() : workers_stop_(false) {
    workers_.reserve(MAX_WORKERS);
    for (size_t i = 0; i < MIN_WORKERS; i++) {
        workers_.emplace_back([this] { start_worker(); });
    }
}

WorkerPool::~WorkerPool() {
    workers_stop_ = true;
    workers_cv_.notify_all();
    for (auto& w : workers_) {
        if (w.joinable()) {
            w.join();
        }
    }
}

void WorkerPool::handle(int sock) {
    {
        std::scoped_lock<std::mutex> lock(connections_mutex_);
        connections_.emplace(std::make_shared<Connection>(
            sock,
            [this](Id id, std::shared_ptr<Connection> conn) {
                assign_connection(id, conn);
            },
            [this](Id id, std::shared_ptr<Connection> conn) {
                unassign_connection(id, conn);
            },
            [this](Id id, const Data& data) { broadcast(id, data); }));
    }
    workers_cv_.notify_one();
    adjust_workers();
}
