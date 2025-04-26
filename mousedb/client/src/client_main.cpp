#include <quill/Backend.h>
#include <quill/Frontend.h>
#include <quill/LogMacros.h>
#include <quill/Logger.h>
#include <quill/sinks/ConsoleSink.h>

#include <bit>
#include <boost/program_options.hpp>
#include <chrono>
#include <cstring>
#include <iostream>
#include <random>
#include <thread>

#include "client.hpp"

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

// Endian helpers
template <typename T>
T native_to_big(T val) {
    if constexpr (std::endian::native == std::endian::big) {
        return val;
    } else {
        if constexpr (sizeof(T) == 2)
            return __builtin_bswap16(val);
        else if constexpr (sizeof(T) == 4)
            return __builtin_bswap32(val);
        else if constexpr (sizeof(T) == 8)
            return __builtin_bswap64(val);
        else
            static_assert(sizeof(T) == 2 || sizeof(T) == 4 || sizeof(T) == 8,
                          "Unsupported");
    }
}

template <typename T>
T big_to_native(T val) {
    return native_to_big(val);
}

// Message methods
std::vector<asio::const_buffer> Message::to_buffers() {
    static uint16_t rsvd = 0;
    uint32_t len_be = native_to_big<uint32_t>(uint32_t(payload.size() + 4));
    uint16_t typ_be = native_to_big<uint16_t>(uint16_t(type));
    std::memcpy(hdr_.data(), &len_be, 4);
    std::memcpy(hdr_.data() + 4, &typ_be, 2);
    std::memcpy(hdr_.data() + 6, &rsvd, 2);
    return {asio::buffer(hdr_), asio::buffer(payload)};
}

void write_msg(tcp::socket &sock, Message &&m) {
    asio::write(sock, m.to_buffers());
}

std::pair<FrameType, std::vector<uint8_t>> read_msg(tcp::socket &sock) {
    std::array<uint8_t, 4> lenb;
    asio::read(sock, asio::buffer(lenb));
    uint32_t len = big_to_native(*reinterpret_cast<uint32_t *>(lenb.data()));

    std::array<uint8_t, 2> typb, rsv;
    asio::read(sock, asio::buffer(typb));
    asio::read(sock, asio::buffer(rsv));
    FrameType typ = static_cast<FrameType>(
        big_to_native(*reinterpret_cast<uint16_t *>(typb.data())));

    std::vector<uint8_t> body(len - 4);
    asio::read(sock, asio::buffer(body));
    return {typ, std::move(body)};
}

int main(int argc, char **argv) {
    namespace po = boost::program_options;
    std::string host = "127.0.0.1";
    int port;
    po::options_description desc("client options");
    desc.add_options()("host",
                       po::value<std::string>(&host)->default_value(host),
                       "server host")("port", po::value<int>(&port)->required(),
                                      "server port");

    po::variables_map vm;
    try {
        po::store(po::parse_command_line(argc, argv, desc), vm);
        po::notify(vm);
    } catch (std::exception const &e) {
        std::cerr << e.what() << "\n\n" << desc << '\n';
        return 1;
    }

    asio::io_context io;
    tcp::resolver resolver(io);
    tcp::socket sock(io);
    asio::connect(sock, resolver.resolve(host, std::to_string(port)));

    // IDENTIFY
    {
        Message id;
        id.type = FrameType::IDENTIFY;
        std::string client_tag = "kv_client";
        uint16_t h_be = native_to_big<uint16_t>(client_tag.size());
        uint16_t p_be = native_to_big<uint16_t>(0);
        id.payload.reserve(2 + client_tag.size() + 2);
        id.payload.insert(id.payload.end(), reinterpret_cast<uint8_t *>(&h_be),
                          reinterpret_cast<uint8_t *>(&h_be) + 2);
        id.payload.insert(id.payload.end(), client_tag.begin(),
                          client_tag.end());
        id.payload.insert(id.payload.end(), reinterpret_cast<uint8_t *>(&p_be),
                          reinterpret_cast<uint8_t *>(&p_be) + 2);
        write_msg(sock, std::move(id));
    }

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<> op_dist(0.0, 1.0);
    constexpr int KEY_SPACE = 10;
    std::uniform_int_distribution<> key_dist(0, KEY_SPACE - 1);
    std::uniform_int_distribution<> val_dist(0, 100);

    while (true) {
        double roll = op_dist(gen);
        std::string key = "key" + std::to_string(key_dist(gen));

        if (roll < 0.7) {
            // WRITE
            std::string value = std::to_string(val_dist(gen));
            Message m;
            m.type = FrameType::WRITE_REQ;
            uint16_t k_be = native_to_big<uint16_t>(key.size());
            uint32_t v_be = native_to_big<uint32_t>(value.size());
            m.payload.reserve(2 + key.size() + 4 + value.size());
            m.payload.insert(m.payload.end(),
                             reinterpret_cast<uint8_t *>(&k_be),
                             reinterpret_cast<uint8_t *>(&k_be) + 2);
            m.payload.insert(m.payload.end(), key.begin(), key.end());
            m.payload.insert(m.payload.end(),
                             reinterpret_cast<uint8_t *>(&v_be),
                             reinterpret_cast<uint8_t *>(&v_be) + 4);
            m.payload.insert(m.payload.end(), value.begin(), value.end());

            write_msg(sock, std::move(m));
            auto [t, body] = read_msg(sock);
            bool ok =
                (t == FrameType::WRITE_RESP && !body.empty() && body[0] == 0);
            LOG_INFO(get_logger(), "PUT {} -> {}", key, ok ? "OK" : "FAIL");
        } else {
            // READ
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
                LOG_INFO(get_logger(), "GET {} -> error", key);
            } else if (body[0] == 1) {
                LOG_INFO(get_logger(), "GET {} -> (not found)", key);
            } else {
                const uint8_t *p = body.data() + 1;
                uint32_t vlen =
                    big_to_native(*reinterpret_cast<const uint32_t *>(p));
                p += 4;
                std::string val(reinterpret_cast<const char *>(p), vlen);
                LOG_INFO(get_logger(), "GET {} -> {}", key, val);
            }
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    }

    return 0;
}
