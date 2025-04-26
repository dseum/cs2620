#include "server.hpp"

#include <quill/LogMacros.h>

namespace asio = boost::asio;
namespace endian = boost::endian;
using tcp = asio::ip::tcp;

std::vector<asio::const_buffer> Message::to_buffers() {
    static uint16_t rsvd = 0;

    uint32_t len_be = endian::native_to_big<uint32_t>(
        static_cast<uint32_t>(payload.size() + 4));
    uint16_t typ_be =
        endian::native_to_big<uint16_t>(static_cast<uint16_t>(type));

    std::memcpy(hdr_.data(), &len_be, 4);
    std::memcpy(hdr_.data() + 4, &typ_be, 2);
    std::memcpy(hdr_.data() + 6, &rsvd, 2);

    return {asio::buffer(hdr_), asio::buffer(payload)};
}

Session::Session(tcp::socket sock, ConnectionManager &mgr, Address self_addr,
                 bool outbound)
    : socket_(std::move(sock)),
      mgr_(mgr),
      strand_(socket_.get_executor()),
      outbound_(outbound),
      self_addr_(std::move(self_addr)),
      remote_{"", 0} {
}

void Session::start() {
    if (outbound_) {
        Message id;
        id.type = FrameType::IDENTIFY;
        uint16_t h_be = endian::native_to_big<uint16_t>(self_addr_.host.size());
        uint16_t p_be = endian::native_to_big<uint16_t>(self_addr_.port);
        id.payload.reserve(2 + self_addr_.host.size() + 2);
        id.payload.insert(id.payload.end(), reinterpret_cast<uint8_t *>(&h_be),
                          reinterpret_cast<uint8_t *>(&h_be) + 2);
        id.payload.insert(id.payload.end(), self_addr_.host.begin(),
                          self_addr_.host.end());
        id.payload.insert(id.payload.end(), reinterpret_cast<uint8_t *>(&p_be),
                          reinterpret_cast<uint8_t *>(&p_be) + 2);
        send(std::move(id));
    }
    asio::async_read(socket_, asio::buffer(hdr_len_),
                     asio::bind_executor(strand_, [self = shared_from_this()](
                                                      auto ec, std::size_t) {
                         if (!ec) self->process_message({});
                     }));
}

void Session::send(Message &&m) {
    asio::post(strand_,
               [self = shared_from_this(), m = std::move(m)]() mutable {
                   bool idle = self->write_q_.empty();
                   self->write_q_.push_back(std::move(m));
                   if (idle) self->do_write();
               });
}

void Session::do_write() {
    asio::async_write(socket_, write_q_.front().to_buffers(),
                      asio::bind_executor(strand_, [self = shared_from_this()](
                                                       auto ec, std::size_t) {
                          if (!ec) {
                              self->write_q_.pop_front();
                              if (!self->write_q_.empty()) self->do_write();
                          } else {
                              self->mgr_.forget(self.get());
                          }
                      }));
}

void Session::process_message(Message &&) {
    uint32_t len = endian::big_to_native<uint32_t>(
        *reinterpret_cast<uint32_t *>(hdr_len_.data()));
    if (len < 4 || len > (1 << 20)) {
        mgr_.forget(this);
        return;
    }

    asio::async_read(
        socket_, asio::buffer(hdr_typ_),
        asio::bind_executor(strand_, [self = shared_from_this(), len](
                                         auto ec, std::size_t) {
            if (ec) {
                self->mgr_.forget(self.get());
                return;
            }

            uint16_t typ_be =
                *reinterpret_cast<uint16_t *>(self->hdr_typ_.data());
            FrameType typ =
                static_cast<FrameType>(endian::big_to_native<uint16_t>(typ_be));

            asio::async_read(
                self->socket_, asio::buffer(self->hdr_rsv_),
                asio::bind_executor(self->strand_, [self, typ, len](
                                                       auto ec2, std::size_t) {
                    if (ec2) {
                        self->mgr_.forget(self.get());
                        return;
                    }

                    self->body_.resize(len - 4);
                    asio::async_read(
                        self->socket_, asio::buffer(self->body_),
                        asio::bind_executor(self->strand_, [self, typ](
                                                               auto ec3,
                                                               std::size_t) {
                            if (ec3) {
                                self->mgr_.forget(self.get());
                                return;
                            }

                            const uint8_t *p = self->body_.data();

                            switch (typ) {
                                case FrameType::IDENTIFY: {
                                    uint16_t hlen =
                                        endian::big_to_native<uint16_t>(
                                            *reinterpret_cast<const uint16_t *>(
                                                p));
                                    p += 2;
                                    std::string host(
                                        reinterpret_cast<const char *>(p),
                                        hlen);
                                    p += hlen;
                                    uint16_t port =
                                        endian::big_to_native<uint16_t>(
                                            *reinterpret_cast<const uint16_t *>(
                                                p));
                                    self->set_remote(
                                        {host, static_cast<int>(port)});
                                    self->mgr_.on_identify(
                                        self.get(),
                                        {host, static_cast<int>(port)});
                                    break;
                                }
                                case FrameType::PEER: {
                                    uint16_t hlen =
                                        endian::big_to_native<uint16_t>(
                                            *reinterpret_cast<const uint16_t *>(
                                                p));
                                    p += 2;
                                    std::string host(
                                        reinterpret_cast<const char *>(p),
                                        hlen);
                                    p += hlen;
                                    uint16_t port =
                                        endian::big_to_native<uint16_t>(
                                            *reinterpret_cast<const uint16_t *>(
                                                p));
                                    self->mgr_.connect_to(
                                        {host, static_cast<int>(port)});
                                    break;
                                }
                                case FrameType::WRITE_REQ: {
                                    uint16_t klen =
                                        endian::big_to_native<uint16_t>(
                                            *reinterpret_cast<const uint16_t *>(
                                                p));
                                    p += 2;
                                    std::string key(
                                        reinterpret_cast<const char *>(p),
                                        klen);
                                    p += klen;
                                    uint32_t vlen =
                                        endian::big_to_native<uint32_t>(
                                            *reinterpret_cast<const uint32_t *>(
                                                p));
                                    p += 4;
                                    std::string val(
                                        reinterpret_cast<const char *>(p),
                                        vlen);
                                    self->mgr_.kv_put(key, val);

                                    Message r;
                                    r.type = FrameType::WRITE_RESP;
                                    r.payload = {0};
                                    self->send(std::move(r));
                                    break;
                                }
                                case FrameType::READ_REQ: {
                                    uint16_t klen =
                                        endian::big_to_native<uint16_t>(
                                            *reinterpret_cast<const uint16_t *>(
                                                p));
                                    p += 2;
                                    std::string key(
                                        reinterpret_cast<const char *>(p),
                                        klen);

                                    auto val = self->mgr_.kv_get(key);
                                    Message r;
                                    r.type = FrameType::READ_RESP;
                                    if (val) {
                                        r.payload.reserve(1 + 4 + val->size());
                                        r.payload.push_back(0);
                                        uint32_t vlen_be =
                                            endian::native_to_big<uint32_t>(
                                                val->size());
                                        r.payload.insert(
                                            r.payload.end(),
                                            reinterpret_cast<uint8_t *>(
                                                &vlen_be),
                                            reinterpret_cast<uint8_t *>(
                                                &vlen_be) +
                                                4);
                                        r.payload.insert(r.payload.end(),
                                                         val->begin(),
                                                         val->end());
                                    } else {
                                        r.payload = {1};
                                    }
                                    self->send(std::move(r));
                                    break;
                                }
                                default:
                                    break;
                            }

                            asio::async_read(
                                self->socket_, asio::buffer(self->hdr_len_),
                                asio::bind_executor(
                                    self->strand_,
                                    [self](auto ec4, std::size_t) {
                                        if (!ec4)
                                            self->process_message({});
                                        else
                                            self->mgr_.forget(self.get());
                                    }));
                        }));
                }));
        }));
}

/* ---------- ConnectionManager -------------------------------------- */
ConnectionManager::ConnectionManager(asio::io_context &io, Address self,
                                     std::optional<Address> join)
    : io_(io),
      acceptor_(io, tcp::endpoint(tcp::v4(), self.port)),
      self_(std::move(self)),
      join_(std::move(join)),
      hb_timer_(io),
      resolver_(asio::make_strand(io)) {
}

void ConnectionManager::run() {
    do_accept();
    if (join_) connect_to(*join_);
    schedule_hb();
}

void ConnectionManager::do_accept() {
    acceptor_.async_accept([this](auto ec, tcp::socket s) {
        if (!ec) {
            auto sess =
                std::make_shared<Session>(std::move(s), *this, self_, false);
            {
                std::lock_guard lk(mtx_);
                sessions_.emplace(sess.get(), sess);
            }
            sess->start();
            LOG_INFO(get_logger(), "ACCEPT → awaiting IDENTIFY");
        }
        do_accept();
    });
}

void ConnectionManager::connect_to(Address const &a) {
    if (a == self_ || connected(a) || a < self_) return;

    resolver_.async_resolve(
        a.host, std::to_string(a.port),
        [this, a](auto ec, tcp::resolver::results_type r) {
            if (ec) {
                LOG_INFO(get_logger(), "resolve {} failed: {}", a.host,
                         ec.message());
                return;
            }

            auto sock = std::make_shared<tcp::socket>(io_);
            asio::async_connect(
                *sock, r, [this, sock, a](auto ec2, tcp::endpoint) {
                    if (ec2) {
                        LOG_INFO(get_logger(), "connect {} failed: {}", a.host,
                                 ec2.message());
                        return;
                    }

                    auto sess = std::make_shared<Session>(std::move(*sock),
                                                          *this, self_, true);
                    sess->set_remote(a);
                    {
                        std::lock_guard lk(mtx_);
                        sessions_.emplace(sess.get(), sess);
                    }
                    sess->start();
                    LOG_INFO(get_logger(), "CONNECTED → {}:{}", a.host, a.port);
                });
        });
}

bool ConnectionManager::connected(Address const &a) const {
    std::lock_guard lk(mtx_);
    for (auto const &[_, s] : sessions_)
        if (s->remote() == a) return true;
    return false;
}

void ConnectionManager::on_identify(Session *s, Address const &addr) {
    s->set_remote(addr);
    LOG_INFO(get_logger(), "IDENTIFY ← {}:{}", addr.host, addr.port);
    broadcast_peer(addr, s);
}

void ConnectionManager::schedule_hb() {
    hb_timer_.expires_after(std::chrono::seconds(2));
    hb_timer_.async_wait([this](auto) {
        Message hb;
        hb.type = FrameType::HEARTBEAT;
        send_all(hb);

        std::size_t peer_cnt = 0;
        {
            std::lock_guard lk(mtx_);
            for (auto const &[_, s] : sessions_)
                if (s->is_server_peer()) ++peer_cnt;
        }
        LOG_INFO(get_logger(), "heartbeat to {} peer(s)", peer_cnt);
        schedule_hb();
    });
}

void ConnectionManager::send_peer(Address const &a,
                                  std::shared_ptr<Session> const &to) {
    Message m;
    m.type = FrameType::PEER;
    uint16_t h_be = endian::native_to_big<uint16_t>(a.host.size());
    uint16_t p_be = endian::native_to_big<uint16_t>(a.port);
    m.payload.reserve(2 + a.host.size() + 2);
    m.payload.insert(m.payload.end(), reinterpret_cast<uint8_t *>(&h_be),
                     reinterpret_cast<uint8_t *>(&h_be) + 2);
    m.payload.insert(m.payload.end(), a.host.begin(), a.host.end());
    m.payload.insert(m.payload.end(), reinterpret_cast<uint8_t *>(&p_be),
                     reinterpret_cast<uint8_t *>(&p_be) + 2);
    to->send(std::move(m));
}

void ConnectionManager::broadcast_peer(Address const &newcomer, Session *skip) {
    std::lock_guard lk(mtx_);
    for (auto const &[ptr, s] : sessions_)
        if (ptr != skip) send_peer(newcomer, s);

    auto it = sessions_.find(skip);
    if (it != sessions_.end()) {
        auto const &new_sess = it->second;
        for (auto const &[ptr, s] : sessions_)
            if (ptr != skip) send_peer(s->remote(), new_sess);
    }
}

void ConnectionManager::send_all(Message const &m) {
    std::lock_guard lk(mtx_);
    for (auto const &[_, s] : sessions_)
        if (s->is_server_peer()) s->send(Message(m));
}

void ConnectionManager::forget(Session *s) {
    std::lock_guard lk(mtx_);
    sessions_.erase(s);
}

/* ---------- KV helpers --------------------------------------------- */
void ConnectionManager::kv_put(std::string const &k, std::string const &v) {
    std::unique_lock lk(kv_mtx_);
    kv_[k] = v;
}

std::optional<std::string> ConnectionManager::kv_get(std::string const &k) {
    std::shared_lock lk(kv_mtx_);
    auto it = kv_.find(k);
    return it == kv_.end() ? std::nullopt
                           : std::optional<std::string>(it->second);
}
