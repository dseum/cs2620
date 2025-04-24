#ifndef SERVER_HPP
#define SERVER_HPP

#include <array>
#include <boost/asio.hpp>
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
    PEER = 3
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
    Session(boost::asio::ip::tcp::socket sock, ConnectionManager &mgr,
            Address self_addr, bool outbound);

    void start();
    void send(Message &&m);

    Address const &remote() const {
        return remote_;
    }

    void set_remote(Address const &a) {
        remote_ = a;
    }

   private:
    using tcp = boost::asio::ip::tcp;
    using strand_t = boost::asio::strand<boost::asio::any_io_executor>;

    void do_write();
    void process_message(Message &&m);

    tcp::socket socket_;
    ConnectionManager &mgr_;
    strand_t strand_;
    bool outbound_;

    Address self_addr_;
    Address remote_;

    std::array<char, 4> hdr_len_;
    std::array<char, 2> hdr_hdr_;
    std::array<uint8_t, 2> hdr_reserved_;
    std::vector<uint8_t> body_;
    std::deque<Message> write_q_;
};

class ConnectionManager {
   public:
    ConnectionManager(boost::asio::io_context &io, Address self,
                      std::optional<Address> join);

    void run();
    void connect_to(Address const &a);
    void forget(Session *s);
    void on_identify(Session *s, Address const &listen_addr);

   private:
    using tcp = boost::asio::ip::tcp;

    void do_accept();
    void schedule_hb();
    bool connected(Address const &a) const;
    void send_all(Message const &m);

    void send_peer(Address const &a, std::shared_ptr<Session> const &to);
    void broadcast_peer(Address const &newcomer, Session *skip);

    boost::asio::io_context &io_;
    tcp::acceptor acceptor_;
    Address self_;
    std::optional<Address> join_;

    mutable std::mutex mtx_;
    std::unordered_map<Session *, std::shared_ptr<Session>> sessions_;

    boost::asio::steady_timer hb_timer_;
    tcp::resolver resolver_;
};

#endif  // SERVER_HPP
