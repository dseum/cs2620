#include <boost/algorithm/string.hpp>
#include <boost/program_options.hpp>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include "database.hpp"
#include "server.hpp"

namespace po = boost::program_options;

void print_main_help(const po::options_description& desc) {
    std::cout << "usage: exe <command> [options]\n\n";
    std::cout << "commands:\n";
    std::cout << "  serve\n";
    std::cout << "  migrate\n\n";
    std::cout << desc << std::endl;
}

void print_serve_help(const po::options_description& desc) {
    std::cout << "usage: exe serve [options]\n\n" << desc << std::endl;
}

void print_migrate_help(const po::options_description& desc) {
    std::cout << "usage: exe migrate [options]\n\n" << desc << std::endl;
}

void handle_serve(const po::variables_map& vm) {
    std::string host = vm["host"].as<std::string>();
    int port = vm["port"].as<int>();
    // Create and start the server (assuming Server's constructor starts it)
    Server server(host, port);
}

void handle_migrate(const po::variables_map& vm) {
    Db db;

    std::ifstream file("main.sql");
    if (!file.is_open()) {
        throw std::runtime_error("main.sql not found");
    }

    std::stringstream buf;
    buf << file.rdbuf();
    std::string stmt;
    while (std::getline(buf, stmt, ';')) {
        boost::trim_right(stmt);
        if (!stmt.empty()) {
            db.execute(stmt + ";");
        }
    }
}

int main(int argc, char* argv[]) {
    try {
        // Global options
        po::options_description global_desc("Global Options");
        global_desc.add_options()("help", "show help");

        // Serve options
        po::options_description serve_desc("Serve Options");
        serve_desc.add_options()("help", "show help")(
            "host,h", po::value<std::string>()->default_value("0.0.0.0"),
            "set server host")("port,p", po::value<int>()->default_value(54100),
                               "set server port");

        // Migrate options
        po::options_description migrate_desc("Migrate Options");
        migrate_desc.add_options()("help", "show help");

        // Parse global options
        po::variables_map vm;
        po::parsed_options parsed = po::command_line_parser(argc, argv)
                                        .options(global_desc)
                                        .allow_unregistered()
                                        .run();
        po::store(parsed, vm);
        po::notify(vm);

        if (argc < 2) {
            print_main_help(global_desc);
            return 1;
        }

        // Determine subcommand
        std::string subcommand = argv[1];
        std::vector<std::string> sub_args(argv + 2, argv + argc);

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
        } else if (subcommand == "migrate") {
            po::variables_map migrate_vm;
            po::store(
                po::command_line_parser(sub_args).options(migrate_desc).run(),
                migrate_vm);
            po::notify(migrate_vm);

            if (migrate_vm.count("help")) {
                print_migrate_help(migrate_desc);
                return 0;
            }

            handle_migrate(migrate_vm);
        } else {
            print_main_help(global_desc);
            return 1;
        }
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
