#ifndef CLIENT_HPP
#define CLIENT_HPP

#include <array>
#include <boost/asio.hpp>
#include <cstdint>
#include <string>
#include <vector>

quill::Logger *get_logger();

template <typename T>
T native_to_big(T val);

template <typename T>
T big_to_native(T val);

namespace asio = boost::asio;
using tcp = asio::ip::tcp;

enum class FrameType : uint16_t {
    IDENTIFY = 0,
    WRITE_REQ = 4,
    WRITE_RESP = 5,
    READ_REQ = 6,
    READ_RESP = 7
};

struct Message {
    FrameType type{};
    std::vector<uint8_t> payload;
    std::array<uint8_t, 8> hdr_{};

    std::vector<asio::const_buffer> to_buffers();
};

void write_msg(tcp::socket &sock, Message &&m);
std::pair<FrameType, std::vector<uint8_t>> read_msg(tcp::socket &sock);

#endif  // CLIENT_HPP
