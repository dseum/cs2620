#include <grpcpp/grpcpp.h>

#include <boost/program_options.hpp>
#include <converse/logging/core.hpp>
#include <string>
#include <vector>

#include "server.hpp"

namespace po = boost::program_options;
namespace lg = converse::logging;

void print_main_help(const po::options_description &desc) {
    std::cout << "usage: exe <command> [options]\n\n";
    std::cout << "commands:\n";
    std::cout << "  serve\n\n";
    std::cout << desc << std::endl;
    std::cout << "\nEXAMPLES:\n";
    std::cout << "  # Start a single server on port 50051:\n";
    std::cout << "    ./exe serve --port=50051\n\n";
    std::cout << "  # If you want 3 servers, open 3 terminals:\n";
    std::cout << "    Terminal 1: ./exe serve --port=50051\n";
    std::cout << "    Terminal 2: ./exe serve --port=50052\n";
    std::cout << "    Terminal 3: ./exe serve --port=50053\n\n";
}

void print_serve_help(const po::options_description &desc) {
    std::cout << "usage: exe serve [options]\n\n" << desc << std::endl;
    std::cout << "\nEXAMPLES:\n";
    std::cout << "  ./exe serve --name=leader --host=0.0.0.0 --port=50051\n";
    std::cout << "  ./exe serve --name=replica1 --host=0.0.0.0 --port=50052\n";
}

void handle_serve(const po::variables_map &vm) {
    std::string name = vm["name"].as<std::string>();
    std::string host = vm["host"].as<std::string>();
    int port = vm["port"].as<int>();
    std::string join_address_as_string = vm["join"].as<std::string>();
    converse::server::Address my_address(host, port);
    std::optional<converse::server::Address> join_address = std::nullopt;
    if (!join_address_as_string.empty()) {
        join_address = converse::server::Address(
            join_address_as_string.substr(0, join_address_as_string.find(':')),
            std::stoi(join_address_as_string.substr(
                join_address_as_string.find(':') + 1,
                join_address_as_string.length())));
    }
    try {
        converse::server::Server server(name, my_address, join_address);
    } catch (const std::exception &e) {
        lg::write(lg::level::error, "server error: {}", e.what());
    }
}

int main(int argc, char *argv[]) {
    lg::init(lg::level::trace, lg::sink_type::console);
    try {
        // Global options
        po::options_description global_desc("Global Options");
        global_desc.add_options()("help", "show help");

        // Serve options
        po::options_description serve_desc("Serve Options");
        serve_desc.add_options()("help", "show help")(
            "name,n", po::value<std::string>()->default_value("main"),
            "set server name (used for database filename)")(
            "host,h", po::value<std::string>()->default_value("0.0.0.0"),
            "set server host")("port,p", po::value<int>()->default_value(50051),
                               "set server port")(
            "join,j", po::value<std::string>()->default_value(""),
            "join an existing cluster via host:port of the leader");

        std::string subcommand;
        std::vector<std::string> sub_args;
        if (argc < 2) {
            subcommand = "serve";
        } else {
            subcommand = argv[1];
            sub_args.assign(argv + 2, argv + argc);
        }

        if (subcommand == "serve") {
            po::variables_map serve_vm;
            po::store(
                po::command_line_parser(sub_args).options(serve_desc).run(),
                serve_vm);
            po::notify(serve_vm);

            if (serve_vm.count("help")) {
                print_serve_help(serve_desc);
                return 0;
            }

            handle_serve(serve_vm);
        } else {
            print_main_help(global_desc);
            return 1;
        }
    } catch (const std::exception &e) {
        lg::write(lg::level::error, e.what());
        return 1;
    }

    return 0;
}
