#include "server.hpp"

#include <converse/service/link/link.pb.h>
#include <grpcpp/grpcpp.h>

#include <boost/program_options.hpp>
#include <converse/logging/core.hpp>
#include <string>

#include "reader.hpp"
#include "service/health/impl.hpp"
#include "service/link/impl.hpp"
#include "service/main/impl.hpp"

namespace lg = converse::logging;

namespace converse {
namespace server {

Address::Address(const std::string &host, int port) : host(host), port(port) {}

Address::operator std::string() const {
    return std::format("{}:{}", host, port);
}

bool Address::operator<(const Address &rhs) const {
    return std::tie(host, port) < std::tie(rhs.host, rhs.port);
}

bool Address::operator==(const Address &rhs) const {
    return std::tie(host, port) == std::tie(rhs.host, rhs.port);
}

Server::Server(const std::string &name, const Address &my_address,
               const std::optional<Address> &join_address) {
    bool is_leader = !join_address.has_value();

    service::health::Impl healthservice_impl;
    service::link::Impl linkservice_impl(name, my_address);
    service::main::Impl mainservice_impl(
        name, my_address, linkservice_impl.cluster_writers,
        linkservice_impl.cluster_writers_mutex);

    grpc::ServerBuilder builder;
    builder.AddListeningPort(my_address, grpc::InsecureServerCredentials());
    builder.RegisterService(&healthservice_impl);
    builder.RegisterService(&linkservice_impl);
    builder.RegisterService(&mainservice_impl);

    auto server = builder.BuildAndStart();
    lg::write(lg::level::info, "listening on {}", std::string(my_address));
    Address leader = my_address;
    if (is_leader) {
        lg::write(lg::level::info, "leader {}", name);
    } else {
        leader = join_address.value();
        std::string leader_as_string(leader);
        lg::write(lg::level::info, "replica {} connecting to {}", name,
                  leader_as_string);
        std::shared_ptr<grpc::Channel> channel = grpc::CreateChannel(
            leader_as_string, grpc::InsecureChannelCredentials());
        std::unique_ptr<service::link::LinkService::Stub> stub =
            service::link::LinkService::NewStub(channel);
        // identifies itself to leader and gets known servers
        {
            service::link::IdentifyMyselfRequest req;
            req.set_host(my_address.host);
            req.set_port(my_address.port);
            service::link::IdentifyMyselfResponse res;
            grpc::ClientContext ctx;
            grpc::Status status = stub->IdentifyMyself(&ctx, req, &res);
            if (!status.ok()) {
                throw std::runtime_error(std::format(
                    "IdentifyMyself failed: {}", status.error_message()));
            }
            lg::write(lg::level::info, "IdentifyMyself ok");
            {
                std::unique_lock lock(linkservice_impl.cluster_addresses_mutex);
                for (const auto &addr : res.cluster_addresses()) {
                    linkservice_impl.cluster_addresses.emplace_back(
                        addr.host(), addr.port());
                    lg::write(lg::level::info, "added server {}:{}",
                              addr.host(), addr.port());
                }
            }
            auto what = res.database();
            linkservice_impl.db->set_bytes(what);
        }
        do {
            service::link::GetTransactionsRequest req;
            auto GetTransactions_reader =
                service::link::reader::GetTransactionsReader(
                    stub.get(), req,
                    [&](const grpc::Status &status,
                        const service::link::GetTransactionsResponse
                            &response) {
                        if (status.ok()) {
                            for (const auto &op : response.operations()) {
                                // Updated to use table_name() as defined in the
                                // proto.
                                const std::string &table = op.table_name();

                                if (op.type() ==
                                    service::link::OPERATION_TYPE_INSERT) {
                                    std::vector<std::string> columns;
                                    std::vector<std::string> placeholders;
                                    std::vector<std::string> values;

                                    for (const auto &entry : op.new_values()) {
                                        columns.push_back(entry.first);
                                        placeholders.push_back("?");
                                        values.push_back(entry.second);
                                    }

                                    std::stringstream sql;
                                    sql << "INSERT INTO " << table << " (";
                                    for (size_t i = 0; i < columns.size();
                                         ++i) {
                                        sql << columns[i];
                                        if (i + 1 < columns.size()) sql << ", ";
                                    }
                                    sql << ") VALUES (";
                                    for (size_t i = 0; i < placeholders.size();
                                         ++i) {
                                        sql << placeholders[i];
                                        if (i + 1 < placeholders.size())
                                            sql << ", ";
                                    }
                                    sql << ")";

                                    std::cout << "[SQL:INSERT] " << sql.str()
                                              << "\n";
                                    linkservice_impl.db->execute(sql.str(),
                                                                 values);
                                } else if (op.type() ==
                                           service::link::
                                               OPERATION_TYPE_UPDATE) {
                                    std::vector<std::string> setClauses;
                                    std::vector<std::string> setValues;
                                    for (const auto &entry : op.new_values()) {
                                        setClauses.push_back(entry.first +
                                                             " = ?");
                                        setValues.push_back(entry.second);
                                    }

                                    std::vector<std::string> whereClauses;
                                    std::vector<std::string> whereValues;
                                    for (const auto &entry :
                                         op.old_key_values()) {
                                        whereClauses.push_back(entry.first +
                                                               " = ?");
                                        whereValues.push_back(entry.second);
                                    }

                                    std::stringstream sql;
                                    sql << "UPDATE " << table << " SET ";
                                    for (size_t i = 0; i < setClauses.size();
                                         ++i) {
                                        sql << setClauses[i];
                                        if (i + 1 < setClauses.size())
                                            sql << ", ";
                                    }

                                    if (!whereClauses.empty()) {
                                        sql << " WHERE ";
                                        for (size_t i = 0;
                                             i < whereClauses.size(); ++i) {
                                            sql << whereClauses[i];
                                            if (i + 1 < whereClauses.size())
                                                sql << " AND ";
                                        }
                                    }

                                    std::vector<std::string> bound;
                                    bound.insert(bound.end(), setValues.begin(),
                                                 setValues.end());
                                    bound.insert(bound.end(),
                                                 whereValues.begin(),
                                                 whereValues.end());

                                    std::cout << "[SQL:UPDATE] " << sql.str()
                                              << "\n";
                                    linkservice_impl.db->execute(sql.str(),
                                                                 bound);
                                } else {
                                    throw std::runtime_error(
                                        "Unsupported operation type");
                                }
                            }
                        }
                    });
            GetTransactions_reader.Await();
            std::unique_lock lock(linkservice_impl.cluster_addresses_mutex);
            Address addr = linkservice_impl.cluster_addresses.front();
            linkservice_impl.cluster_addresses.pop_front();
            if (addr == my_address) {
                is_leader = true;
                lg::write(lg::level::info, "i am now the leader");
            } else {
                lg::write(lg::level::info, "promoted replica {} to leader",
                          std::string(addr));
                channel = grpc::CreateChannel(
                    std::string(addr), grpc::InsecureChannelCredentials());
                stub = service::link::LinkService::NewStub(channel);
            }
        } while (!is_leader);
    }

    // is leader
    server->Wait();
}
}  // namespace server
}  // namespace converse
