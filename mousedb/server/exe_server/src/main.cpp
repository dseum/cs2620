#include "server.hpp"

int main(int argc, char **argv) {
    boost::program_options::options_description desc("options");
    std::string host = "0.0.0.0", join;
    int port;
    desc.add_options()("port",
                       boost::program_options::value<int>(&port)->required(),
                       "listen port")(
        "host",
        boost::program_options::value<std::string>(&host)->default_value(host),
        "bind address")("join",
                        boost::program_options::value<std::string>(&join),
                        "leader host:port");
    boost::program_options::variables_map vm;
    store(parse_command_line(argc, argv, desc), vm);
    try {
        notify(vm);
    } catch (std::exception const &e) {
        std::cerr << e.what() << "\n" << desc;
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
    ConnectionManager cm(io, {host, port}, join_addr);
    cm.run();

    std::vector<std::jthread> workers;
    for (unsigned i = 1; i < std::thread::hardware_concurrency(); ++i)
        workers.emplace_back([&] { io.run(); });
    io.run();
}
