#include <quill/LogMacros.h>

#include <boost/asio.hpp>
#include <boost/program_options.hpp>
#include <iostream>

#include "server.hpp"

namespace po = boost::program_options;
namespace asio = boost::asio;

int main(int argc, char **argv) {
    std::string host = "0.0.0.0", join;
    int port;

    po::options_description desc("server options");
    desc.add_options()("port", po::value<int>(&port)->required(),
                       "listen port")(
        "host", po::value<std::string>(&host)->default_value(host),
        "bind address")("join", po::value<std::string>(&join),
                        "leader host:port");

    po::variables_map vm;
    try {
        po::store(po::parse_command_line(argc, argv, desc), vm);
        po::notify(vm);
    } catch (std::exception const &e) {
        std::cerr << e.what() << "\n\n" << desc << '\n';
        return 1;
    }

    std::optional<Address> join_addr;
    if (vm.count("join")) {
        auto p = join.find(':');
        if (p == std::string::npos) {
            std::cerr << "--join host:port\n";
            return 1;
        }
        join_addr = Address{join.substr(0, p), std::stoi(join.substr(p + 1))};
    }

    asio::io_context io;
    Address self{host, port};
    ConnectionManager cm(io, self, join_addr);

    LOG_INFO(get_logger(), "starting node {}:{}", host, port);
    cm.run();

    auto n = std::thread::hardware_concurrency();
    std::vector<std::thread> workers;
    workers.reserve(n ? n - 1 : 1);
    for (unsigned i = 1; i < n; ++i) workers.emplace_back([&] { io.run(); });
    io.run();
    for (auto &t : workers) t.join();
}
