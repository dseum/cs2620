#include "server.hpp"

#include <boost/program_options.hpp>
#include <iostream>
#include <thread>

namespace asio = boost::asio;
namespace endian = boost::endian;
using tcp = asio::ip::tcp;

std::vector<asio::const_buffer> Message::to_buffers() const {
    static uint16_t reserved = 0;
    uint32_t len_be = endian::native_to_big<uint32_t>(
        static_cast<uint32_t>(payload.size() + 4));
    uint16_t typ_be =
        endian::native_to_big<uint16_t>(static_cast<uint16_t>(type));
    return {asio::buffer(&len_be, 4), asio::buffer(&typ_be, 2),
            asio::buffer(&reserved, 2), asio::buffer(payload)};
}

Session::Session(tcp::socket sock, ConnectionManager &m, bool outbound)
    : socket_(std::move(sock)),
      mgr_(m),
      strand_(socket_.get_executor()),
      outbound_(outbound) {
}

void Session::start() {
    if (outbound_) { /* send IDENTIFY once TCP is up */
        Message id;
        id.type = FrameType::IDENTIFY;

        const std::string host = socket_.local_endpoint().address().to_string();
        uint16_t host_len_be = endian::native_to_big<uint16_t>(host.size());
        uint16_t port_be =
            endian::native_to_big<uint16_t>(socket_.local_endpoint().port());
        const std::string name = "node";
        uint16_t name_len_be = endian::native_to_big<uint16_t>(name.size());

        id.payload.reserve(2 + host.size() + 2 + 2 + name.size());
        id.payload.insert(id.payload.end(),
                          reinterpret_cast<uint8_t *>(&host_len_be),
                          reinterpret_cast<uint8_t *>(&host_len_be) + 2);
        id.payload.insert(id.payload.end(), host.begin(), host.end());
        id.payload.insert(id.payload.end(),
                          reinterpret_cast<uint8_t *>(&port_be),
                          reinterpret_cast<uint8_t *>(&port_be) + 2);
        id.payload.insert(id.payload.end(),
                          reinterpret_cast<uint8_t *>(&name_len_be),
                          reinterpret_cast<uint8_t *>(&name_len_be) + 2);
        id.payload.insert(id.payload.end(), name.begin(), name.end());

        send(std::move(id));
    }

    // starts read loop
    asio::async_read(
        socket_, asio::buffer(hdr_len_),
        asio::bind_executor(
            strand_, [self = shared_from_this()](boost::system::error_code ec,
                                                 std::size_t) {
                if (!ec) self->process_message({});
            }));
}

void Session::send(Message &&m) {
    asio::dispatch(strand_,
                   [self = shared_from_this(), m = std::move(m)]() mutable {
                       bool was_idle = self->write_q_.empty();
                       self->write_q_.push_back(std::move(m));
                       if (was_idle) self->do_write();
                   });
}

void Session::do_write() {
    asio::async_write(
        socket_, write_q_.front().to_buffers(),
        asio::bind_executor(
            strand_, [self = shared_from_this()](boost::system::error_code ec,
                                                 std::size_t) {
                if (!ec) {
                    self->write_q_.pop_front();
                    if (!self->write_q_.empty()) self->do_write();
                } else {
                    self->mgr_.forget(self.get());
                }
            }));
}

/* very small header‑parser / dispatcher (identical to earlier version) */
void Session::process_message(
    Message &&dummy) { /* dummy only to enter strand */
    /* read header length already in hdr_len_ */
    uint32_t len = endian::big_to_native<uint32_t>(
        *reinterpret_cast<uint32_t *>(hdr_len_.data()));
    if (len < 4 || len > (1 << 20)) {
        mgr_.forget(this);
        return;
    }

    /* read type+flags then body */
    asio::async_read(
        socket_, asio::buffer(hdr_type_),
        asio::bind_executor(
            strand_, [self = shared_from_this(), len](
                         boost::system::error_code ec, std::size_t) {
                if (ec) {
                    self->mgr_.forget(self.get());
                    return;
                }
                uint16_t typ = endian::big_to_native<uint16_t>(
                    *reinterpret_cast<uint16_t *>(self->hdr_type_.data()));
                self->body_.resize(len - 4);
                asio::async_read(
                    self->socket_, asio::buffer(self->body_),
                    asio::bind_executor(
                        self->strand_,
                        [self, typ](boost::system::error_code ec, std::size_t) {
                            if (ec) {
                                self->mgr_.forget(self.get());
                                return;
                            }
                            Message m;
                            m.type = static_cast<FrameType>(typ);
                            m.payload.swap(self->body_);

                            /* ===== handle the frame ===== */
                            if (m.type == FrameType::IDENTIFY) {
                                std::cout << "IDENTIFY received ("
                                          << m.payload.size() << " bytes)\n";
                            } else if (m.type == FrameType::WAL_APPEND) {
                                std::cout << "WAL_APPEND received ("
                                          << m.payload.size() << " bytes)\n";
                            }
                            /* loop */
                            asio::async_read(
                                self->socket_, asio::buffer(self->hdr_len_),
                                asio::bind_executor(
                                    self->strand_,
                                    [self](boost::system::error_code ec,
                                           std::size_t) {
                                        if (!ec)
                                            self->process_message({});
                                        else
                                            self->mgr_.forget(self.get());
                                    }));
                        }));
            }));
}

ConnectionManager::ConnectionManager(asio::io_context &io, Address self,
                                     std::optional<Address> join)
    : io_(io),
      acceptor_(io, tcp::endpoint(tcp::v4(), self.port)),
      self_(std::move(self)),
      join_(std::move(join)),
      hb_timer_(io) {
}

void ConnectionManager::run() {
    do_accept();
    if (join_) connect_to(*join_);
    schedule_hb();
}

void ConnectionManager::do_accept() {
    acceptor_.async_accept([this](boost::system::error_code ec, tcp::socket s) {
        if (!ec) {
            auto sess = std::make_shared<Session>(std::move(s), *this, false);
            {
                std::lock_guard lk(mtx_);
                sessions_.emplace(sess.get(), sess);
            }
            sess->start();
        }
        do_accept();
    });
}

void ConnectionManager::connect_to(Address const &a) {
    tcp::resolver res(io_);
    res.async_resolve(
        a.host, std::to_string(a.port),
        [this](boost::system::error_code ec, tcp::resolver::results_type r) {
            if (ec) return;
            auto sock = std::make_shared<tcp::socket>(io_);
            asio::async_connect(
                *sock, r,
                [this, sock](boost::system::error_code ec, tcp::endpoint) {
                    if (ec) return;
                    auto sess = std::make_shared<Session>(std::move(*sock),
                                                          *this, true);
                    {
                        std::lock_guard lk(mtx_);
                        sessions_.emplace(sess.get(), sess);
                    }
                    sess->start();
                });
        });
}

void ConnectionManager::send_all(Message const &m) {
    std::lock_guard lk(mtx_);
    for (auto &[_, s] : sessions_) s->send(Message(m));
}

void ConnectionManager::forget(Session *s) {
    std::lock_guard lk(mtx_);
    sessions_.erase(s);
}

void ConnectionManager::schedule_hb() {
    hb_timer_.expires_after(std::chrono::seconds(2));
    hb_timer_.async_wait([this](boost::system::error_code) {
        Message hb;
        hb.type = FrameType::HEARTBEAT;
        send_all(hb);
        schedule_hb();
    });
}
