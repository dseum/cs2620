#ifndef SERVER_HPP
#define SERVER_HPP

#include <quill/Backend.h>
#include <quill/Frontend.h>
#include <quill/Logger.h>
#include <quill/sinks/ConsoleSink.h>

#include <mutex>

inline quill::Logger *get_logger() {
    static struct Init {
        quill::Logger *lg;

        Init() {
            quill::Backend::start();
            auto sink = quill::Frontend::create_or_get_sink<quill::ConsoleSink>(
                "stdout");
            lg = quill::Frontend::create_or_get_logger("root", std::move(sink));
        }
    } s;

    return s.lg;
}

#include <array>
#include <boost/asio.hpp>
#include <boost/endian/conversion.hpp>
#include <deque>
#include <iostream>
#include <optional>
#include <shared_mutex>
#include <string>
#include <unordered_map>
#include <vector>

struct Address {
    std::string host;
    int port;

    Address(std::string h = "0.0.0.0", int p = 0)
        : host(std::move(h)), port(p) {
    }

    bool operator<(Address const &o) const {
        return (host < o.host) || (host == o.host && port < o.port);
    }

    bool operator==(Address const &o) const {
        return host == o.host && port == o.port;
    }
};

enum class FrameType : uint16_t {
    IDENTIFY = 0,
    WAL_APPEND = 1,
    HEARTBEAT = 2,
    PEER = 3,
    WRITE_REQ = 4,
    WRITE_RESP = 5,
    READ_REQ = 6,
    READ_RESP = 7
};

struct Message {
    FrameType type{};
    std::vector<uint8_t> payload;
    std::array<uint8_t, 8> hdr_{};

    std::vector<boost::asio::const_buffer> to_buffers();
};

class ConnectionManager;

class Session : public std::enable_shared_from_this<Session> {
   public:
    Session(boost::asio::ip::tcp::socket, ConnectionManager &,
            Address self_addr, bool outbound);
    void start();
    void send(Message &&);

    Address const &remote() const {
        return remote_;
    }

    bool is_server_peer() const {
        return remote_.port != 0;
    }

    void set_remote(Address a) {
        remote_ = std::move(a);
    }

   private:
    void do_write();
    void process_message(Message &&);

    boost::asio::ip::tcp::socket socket_;
    ConnectionManager &mgr_;
    boost::asio::strand<boost::asio::any_io_executor> strand_;

    bool outbound_;
    Address self_addr_;
    Address remote_;

    std::array<uint8_t, 4> hdr_len_{};
    std::array<uint8_t, 2> hdr_typ_{};
    std::array<uint8_t, 2> hdr_rsv_{};
    std::vector<uint8_t> body_;
    std::deque<Message> write_q_;
};

class ConnectionManager {
   public:
    ConnectionManager(boost::asio::io_context &, Address self,
                      std::optional<Address> join);
    void run();
    void on_identify(Session *, Address const &);
    void connect_to(Address const &);

    void kv_put(std::string const &, std::string const &);
    std::optional<std::string> kv_get(std::string const &);

    void forget(Session *);
    void send_all(Message const &);

   private:
    bool connected(Address const &) const;
    void schedule_hb();
    void do_accept();
    void broadcast_peer(Address const &, Session *);
    void send_peer(Address const &, std::shared_ptr<Session> const &);

    std::unordered_map<std::string, std::string> kv_;
    mutable std::shared_mutex kv_mtx_;

    boost::asio::io_context &io_;
    boost::asio::ip::tcp::acceptor acceptor_;
    Address self_;
    std::optional<Address> join_;
    boost::asio::steady_timer hb_timer_;
    boost::asio::ip::tcp::resolver resolver_;
    std::unordered_map<Session *, std::shared_ptr<Session>> sessions_;
    mutable std::mutex mtx_;
};

#endif  // SERVER_HPP