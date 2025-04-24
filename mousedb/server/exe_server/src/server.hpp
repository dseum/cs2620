#pragma once

#include <array>
#include <boost/asio.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/steady_timer.hpp>
#include <boost/endian/conversion.hpp>
#include <deque>
#include <mutex>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

struct Address {
    std::string host;
    int port{};

    explicit Address(std::string h = "0.0.0.0", int p = 0)
        : host(std::move(h)), port(p) {
    }

    explicit operator std::string() const {
        return host + ":" + std::to_string(port);
    }

    auto operator<(Address const &o) const {
        return std::tie(host, port) < std::tie(o.host, o.port);
    }

    auto operator==(Address const &o) const {
        return std::tie(host, port) == std::tie(o.host, o.port);
    }
};

enum class FrameType : uint16_t { IDENTIFY = 0, WAL_APPEND = 1, HEARTBEAT = 2 };

struct Message {
    FrameType type{};
    std::vector<uint8_t> payload;

    /* serialise to a buffer sequence */
    std::vector<boost::asio::const_buffer> to_buffers() const;
};

class ConnectionManager;

class Session : public std::enable_shared_from_this<Session> {
   public:
    Session(boost::asio::ip::tcp::socket sock, ConnectionManager &mgr,
            bool outbound);

    void start();           /* begin reading + (optionally) send IDENTIFY   */
    void send(Message &&m); /* enqueue an outgoing message (thread‑safe)    */

   private:
    void do_read_header();
    void process_message(Message &&);

    boost::asio::ip::tcp::socket socket_;
    ConnectionManager &mgr_;
    boost::asio::strand<boost::asio::io_context::executor_type> strand_;

    std::array<uint8_t, 4> hdr_len_{};
    std::array<uint8_t, 4> hdr_type_{};
    std::vector<uint8_t> body_;
    std::deque<Message> write_q_;

    bool outbound_;
};

class ConnectionManager {
   public:
    ConnectionManager(boost::asio::io_context &io, Address self,
                      std::optional<Address> join);

    void run(); /* start accept(), optional connect(), heartbeat */

    /* called by Session */
    void send_all(Message const &m);
    void forget(Session *s);

   private:
    void do_accept();
    void connect_to(Address const &);
    void schedule_hb();

    boost::asio::io_context &io_;
    boost::asio::ip::tcp::acceptor acceptor_;
    Address self_;
    std::optional<Address> join_;

    std::unordered_map<Session *, std::shared_ptr<Session>> sessions_;
    std::mutex mtx_;
    boost::asio::steady_timer hb_timer_;
};
