#include <grpcpp/grpcpp.h>
#include <boost/program_options.hpp>
#include <converse/logging/core.hpp>
#include <iostream>
#include <string>
#include <vector>

#include "server.hpp"
#include "link.hpp"

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
    int port        = vm["port"].as<int>();
    std::string address = std::format("{}:{}", host, port);

    std::string joinAddr = vm["join"].as<std::string>();
    bool isLeader = joinAddr.empty();

    converse::service::main::Impl mainservice_impl(name, host, port);
    converse::service::link::LinkServiceImpl linkservice_impl(name);

    grpc::ServerBuilder builder;
    builder.AddListeningPort(address, grpc::InsecureServerCredentials());
    builder.RegisterService(&mainservice_impl);
    builder.RegisterService(&linkservice_impl);

    auto server = builder.BuildAndStart();
    lg::write(lg::level::info, "listening on {}", address);

    std::string connectTarget = isLeader ? address : joinAddr;
    std::cout << (isLeader ? "[LEADER]" : "[REPLICA]") << " Connecting to " << connectTarget << std::endl;

    auto channel = grpc::CreateChannel(connectTarget, grpc::InsecureChannelCredentials());
    std::unique_ptr<converse::service::link::LinkService::Stub> stub =
        converse::service::link::LinkService::NewStub(channel);

    {
        ::converse::service::link::IdentifyMyselfRequest req;
        req.set_host(host);
        req.set_port(port);

        ::converse::service::link::IdentifyMyselfResponse resp;
        grpc::ClientContext ctx;
        grpc::Status status = stub->IdentifyMyself(&ctx, req, &resp);
        if (!status.ok()) {
            std::cerr << "IdentifyMyself failed: " << status.error_message() << std::endl;
        } else {
            std::cout << "IdentifyMyself OK.\n";
        }
    }

    uint64_t myId = 0;
    {
        ::converse::service::link::ClaimServerIdRequest req;
        req.set_server_id(0);
        ::converse::service::link::ClaimServerIdResponse resp;

        grpc::ClientContext ctx;
        grpc::Status status = stub->ClaimServerId(&ctx, req, &resp);
        if (!status.ok()) {
            std::cerr << "ClaimServerId failed: " << status.error_message() << std::endl;
        } else {
            myId = resp.server_id();
            std::cout << (isLeader ? "[LEADER]" : "[REPLICA]")
                      << " Assigned ID=" << myId << std::endl;
        }
    }

    server->Wait();
}

int main(int argc, char *argv[]) {
    lg::init(lg::level::trace, lg::sink_type::console);
    try {
        // Global options
        po::options_description global_desc("Global Options");
        global_desc.add_options()
            ("help", "show help");

        // Serve options
        po::options_description serve_desc("Serve Options");
        serve_desc.add_options()
            ("help", "show help")
            ("name,n", po::value<std::string>()->default_value("main"),
             "set server name (used for database filename)")
            ("host,h", po::value<std::string>()->default_value("0.0.0.0"),
             "set server host")
            ("port,p", po::value<int>()->default_value(50051),
             "set server port")
            ("join,j", po::value<std::string>()->default_value(""),
            "join an existing cluster via host:port of a leader/replica");
        
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
            po::store(po::command_line_parser(sub_args).options(serve_desc).run(), serve_vm);
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
