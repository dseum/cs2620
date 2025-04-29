#include <quill/Backend.h>
#include <quill/Frontend.h>
#include <quill/LogMacros.h>
#include <quill/Logger.h>
#include <quill/sinks/ConsoleSink.h>

#include <bit>
#include <boost/asio.hpp>
#include <boost/program_options.hpp>
#include <chrono>
#include <cstring>
#include <iostream>
#include <random>
#include <thread>

#include "hlc.hpp"

namespace asio = boost::asio;
using tcp = asio::ip::tcp;

/* --------------- helper: endian swap for portability -------------------- */
template <typename T>
T native_to_big(T val) {
    if constexpr (std::endian::native == std::endian::big) return val;
    if constexpr (sizeof(T) == 2) return __builtin_bswap16(val);
    if constexpr (sizeof(T) == 4) return __builtin_bswap32(val);
    if constexpr (sizeof(T) == 8) return __builtin_bswap64(val);
}

template <typename T>
T big_to_native(T v) {
    return native_to_big(v);
}

/* ------------------------------- Message -------------------------------- */
enum class FrameType : uint16_t {
    IDENTIFY,
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

    std::vector<asio::const_buffer> to_buffers() {
        static uint16_t rsvd = 0;
        uint32_t len_be = native_to_big<uint32_t>(payload.size() + 4);
        uint16_t typ_be = native_to_big<uint16_t>(uint16_t(type));
        std::memcpy(hdr.data(), &len_be, 4);
        std::memcpy(hdr.data() + 4, &typ_be, 2);
        std::memcpy(hdr.data() + 6, &rsvd, 2);
        return {asio::buffer(hdr), asio::buffer(payload)};
    }
};

static void encode_hlc(const HLC &c, std::vector<uint8_t> &out) {
    uint64_t phys_be = native_to_big<uint64_t>(c.physical_us);
    uint16_t log_be = native_to_big<uint16_t>(c.logical);
    uint32_t id_be = native_to_big<uint32_t>(c.node_id);
    out.insert(out.end(), reinterpret_cast<uint8_t *>(&phys_be),
               reinterpret_cast<uint8_t *>(&phys_be) + 8);
    out.insert(out.end(), reinterpret_cast<uint8_t *>(&log_be),
               reinterpret_cast<uint8_t *>(&log_be) + 2);
    out.insert(out.end(), reinterpret_cast<uint8_t *>(&id_be),
               reinterpret_cast<uint8_t *>(&id_be) + 4);
}

static HLC decode_hlc(const uint8_t *p) {
    uint64_t phys = big_to_native(*reinterpret_cast<const uint64_t *>(p));
    uint16_t log = big_to_native(*reinterpret_cast<const uint16_t *>(p + 8));
    uint32_t id = big_to_native(*reinterpret_cast<const uint32_t *>(p + 10));
    return HLC{phys, log, id};
}

quill::Logger *get_logger() {
    static struct Init {
        quill::Logger *lg;

        Init() {
            quill::Backend::start();
            auto sink = quill::Frontend::create_or_get_sink<quill::ConsoleSink>(
                "stdout");
            lg = quill::Frontend::create_or_get_logger("root", std::move(sink));
        }
    } init;

    return init.lg;
}

/* ------------------ tiny sync I/O helpers ------------------------------ */
static void write_msg(tcp::socket &s, Message &&m) {
    asio::write(s, m.to_buffers());
}

static std::pair<FrameType, std::vector<uint8_t>> read_msg(tcp::socket &s) {
    std::array<uint8_t, 4> lenb;
    asio::read(s, asio::buffer(lenb));
    uint32_t len = big_to_native(*reinterpret_cast<uint32_t *>(lenb.data()));
    std::array<uint8_t, 2> typb, rsv;
    asio::read(s, asio::buffer(typb));
    asio::read(s, asio::buffer(rsv));
    FrameType typ =
        FrameType(big_to_native(*reinterpret_cast<uint16_t *>(typb.data())));
    std::vector<uint8_t> body(len - 4);
    asio::read(s, asio::buffer(body));
    return {typ, std::move(body)};
}

int main(int argc, char **argv) {
    namespace po = boost::program_options;
    std::string host = "127.0.0.1";
    int port;
    po::options_description desc("client opts");
    desc.add_options()(
        "host", po::value<std::string>(&host)->default_value(host), "server")(
        "port", po::value<int>(&port)->required(), "port");
    po::variables_map vm;
    try {
        po::store(po::parse_command_line(argc, argv, desc), vm);
        po::notify(vm);
    } catch (std::exception const &e) {
        std::cerr << e.what() << "\n\n" << desc << '\n';
        return 1;
    }

    asio::io_context io;
    tcp::resolver r(io);
    tcp::socket sock(io);
    asio::connect(sock, r.resolve(host, std::to_string(port)));

    /* IDENTIFY */
    {
        Message id;
        id.type = FrameType::IDENTIFY;
        std::string tag = "kv_client";
        uint16_t h_be = native_to_big<uint16_t>(tag.size());
        uint16_t p_be = native_to_big<uint16_t>(0);
        id.payload.reserve(2 + tag.size() + 2);
        id.payload.insert(id.payload.end(), reinterpret_cast<uint8_t *>(&h_be),
                          reinterpret_cast<uint8_t *>(&h_be) + 2);
        id.payload.insert(id.payload.end(), tag.begin(), tag.end());
        id.payload.insert(id.payload.end(), reinterpret_cast<uint8_t *>(&p_be),
                          reinterpret_cast<uint8_t *>(&p_be) + 2);
        write_msg(sock, std::move(id));
    }

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<> op_dist(0.0, 1.0);
    std::uniform_int_distribution<> key_dist(0, 9), val_dist(0, 100);

    while (true) {
        double roll = op_dist(gen);
        std::string key = "key" + std::to_string(key_dist(gen));

        if (roll < 0.7) { /* PUT */
            std::string value = std::to_string(val_dist(gen));
            Message m;
            m.type = FrameType::WRITE_REQ;
            uint16_t k_be = native_to_big<uint16_t>(key.size());
            uint32_t v_be = native_to_big<uint32_t>(value.size());
            m.payload.reserve(2 + key.size() + 4 + value.size() + 14);
            m.payload.insert(m.payload.end(),
                             reinterpret_cast<uint8_t *>(&k_be),
                             reinterpret_cast<uint8_t *>(&k_be) + 2);
            m.payload.insert(m.payload.end(), key.begin(), key.end());
            m.payload.insert(m.payload.end(),
                             reinterpret_cast<uint8_t *>(&v_be),
                             reinterpret_cast<uint8_t *>(&v_be) + 4);
            m.payload.insert(m.payload.end(), value.begin(), value.end());
            /* dummy zero-clock; server stamps real one */
            encode_hlc(HLC{0, 0, 0}, m.payload);

            write_msg(sock, std::move(m));
            auto [t, body] = read_msg(sock);
            bool ok =
                (t == FrameType::WRITE_RESP && !body.empty() && body[0] == 0);
            LOG_INFO(get_logger(), "PUT {} -> {}", key, ok ? "OK" : "FAIL");
        } else { /* GET */
            Message m;
            m.type = FrameType::READ_REQ;
            uint16_t k_be = native_to_big<uint16_t>(key.size());
            m.payload.reserve(2 + key.size());
            m.payload.insert(m.payload.end(),
                             reinterpret_cast<uint8_t *>(&k_be),
                             reinterpret_cast<uint8_t *>(&k_be) + 2);
            m.payload.insert(m.payload.end(), key.begin(), key.end());
            write_msg(sock, std::move(m));
            auto [t, body] = read_msg(sock);
            if (t != FrameType::READ_RESP || body.empty()) {
                LOG_INFO(get_logger(), "GET {} -> err", key);
            } else if (body[0] == 1) {
                LOG_INFO(get_logger(), "GET {} -> (nf)", key);
            } else {
                const uint8_t *p = body.data() + 1;
                uint32_t vlen =
                    big_to_native(*reinterpret_cast<const uint32_t *>(p));
                p += 4;
                std::string val(reinterpret_cast<const char *>(p), vlen);
                p += vlen;
                HLC ts = decode_hlc(p);
                LOG_INFO(get_logger(), "GET {} -> {} @{}+{} (node {})", key,
                         val, ts.physical_us, ts.logical, ts.node_id);
            }
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    }
}
