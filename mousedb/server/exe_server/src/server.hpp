#pragma once
#include <quill/Backend.h>
#include <quill/Frontend.h>
#include <quill/Logger.h>
#include <quill/sinks/ConsoleSink.h>

#include <array>
#include <boost/asio.hpp>
#include <boost/endian/conversion.hpp>
#include <deque>
#include <mousedb/database/core.hpp>
#include <mutex>
#include <optional>
#include <shared_mutex>
#include <string>
#include <unordered_map>
#include <vector>

#include "hlc.hpp"
#include "lww.hpp"

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

/* ------------------------------------------------------------------------- */
struct Address {
    std::string host;
    int port;

    bool operator<(Address const &o) const {
        return (host < o.host) || (host == o.host && port < o.port);
    }

    bool operator==(Address const &o) const {
        return host == o.host && port == o.port;
    }
};

/* ------------------------------------------------------------------------- */
enum class FrameType : uint16_t {
    IDENTIFY = 0,
    WAL_APPEND,
    HEARTBEAT,
    PEER,
    WRITE_REQ,
    WRITE_RESP,
    READ_REQ,
    READ_RESP
};

struct Message {
    FrameType type{};
    std::vector<uint8_t> payload;
    std::array<uint8_t, 8> hdr{};
    std::vector<boost::asio::const_buffer> to_buffers();
};

/* ------------------------------------------------------------------------- */
struct ValueRec {
    std::string value;
    HLC clock;
};

class ConnectionManager;  // fwd

/* ------------------------------------------------------------------------- */
class Session : public std::enable_shared_from_this<Session> {
   public:
    Session(boost::asio::ip::tcp::socket sock, ConnectionManager &mgr,
            Address self_addr, bool outbound);

    void start();
    void send(Message &&m);

    Address const &remote() const {
        return remote_;
    }

    void set_remote(Address a) {
        remote_ = std::move(a);
    }

    bool is_server_peer() const {
        return remote_.port != 0;
    }

   private:
    void do_write();
    void process_message(Message &&);

    boost::asio::ip::tcp::socket socket_;
    ConnectionManager &mgr_;

    using any_strand = boost::asio::strand<boost::asio::any_io_executor>;
    any_strand strand_;

    bool outbound_;
    Address self_addr_;
    Address remote_;

    std::array<uint8_t, 4> hdr_len_{};
    std::array<uint8_t, 2> hdr_typ_{};
    std::array<uint8_t, 2> hdr_rsv_{};
    std::vector<uint8_t> body_;
    std::deque<Message> write_q_;
};

/* ------------------------------------------------------------------------- */
class ConnectionManager {
   public:
    ConnectionManager(boost::asio::io_context &io, Address self,
                      std::optional<Address> join);

    void run();

    /* kv helpers */
    void kv_put(const std::string &k, const std::string &v, const HLC &ts);
    std::optional<ValueRec> kv_get(const std::string &k);

    /* session callbacks */
    void on_identify(Session *s, Address const &addr);
    void forget(Session *s);

    /* utility */
    void connect_to(Address const &a);
    bool connected(Address const &a) const;
    void send_all(Message const &m);

    HybridClock hlc_;  // local HLC

   private:
    void do_accept();
    void send_peer(Address const &a, std::shared_ptr<Session> const &to);
    void broadcast_peer(Address const &newcomer, Session *skip);
    void schedule_hb();

    boost::asio::io_context &io_;
    boost::asio::ip::tcp::acceptor acceptor_;
    Address self_;
    std::optional<Address> join_;
    boost::asio::steady_timer hb_timer_;
    boost::asio::ip::tcp::resolver resolver_;

    mutable std::mutex mtx_;
    std::unordered_map<Session *, std::shared_ptr<Session>> sessions_;

    /* in-memory KV (temporary) */
    mutable std::shared_mutex kv_mtx_;
    std::unordered_map<std::string, ValueRec> kv_;

    mousedb::database::Options db_opts_;
    mousedb::database::Database db_;
};
