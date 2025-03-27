#include <grpcpp/grpcpp.h>

#include <boost/program_options.hpp>
#include <converse/logging/core.hpp>
#include <iostream>
#include <string>
#include <vector>

#include "server.hpp"

namespace po = boost::program_options;
namespace lg = converse::logging;

void print_main_help(const po::options_description &desc) {
    std::cout << "usage: exe <command> [options]\n\n";
    std::cout << "commands:\n";
    std::cout << "  serve\n";
    std::cout << desc << std::endl;
}

void print_serve_help(const po::options_description &desc) {
    std::cout << "usage: exe serve [options]\n\n" << desc << std::endl;
}

void handle_serve(const po::variables_map &vm) {
    std::string name = vm["name"].as<std::string>();
    std::string host = vm["host"].as<std::string>();
    int port = vm["port"].as<int>();
    std::string address(std::format("{}:{}", host, port));

    converse::service::main::Impl mainservice_impl(name);

    grpc::ServerBuilder builder;
    builder.AddListeningPort(address, grpc::InsecureServerCredentials());
    builder.RegisterService(&mainservice_impl);
    auto server = builder.BuildAndStart();

    lg::write(lg::level::info, "listening on {}", address);

    server->Wait();
}

int main(int argc, char *argv[]) {
    lg::init(lg::sink_type::console);
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
                               "set server port");

        // Defaults to "serve" if no subcommand is provided
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
        lg::write(lg::level::error, "{}", e.what());
        return 1;
    }

    return 0;
}
