#pragma once

#include <converse/service/link/link.grpc.pb.h>
#include <converse/service/link/link.pb.h>
#include <grpcpp/grpcpp.h>

#include <shared_mutex>
#include <string>

#include "database.hpp"
#include "server.hpp"

namespace converse {
namespace service {
namespace link {

// The gRPC service implementation for replication.
class Impl final : public LinkService::Service {
   public:
    explicit Impl(const std::string &name, const server::Address &address);
    ~Impl() override;

    grpc::Status GetServers(
        grpc::ServerContext *context, const GetServersRequest *request,
        grpc::ServerWriter<GetServersResponse> *writer) override;

    // RPC: A new server calls this to broadcast its host/port. No ID assigned
    // yet.
    grpc::Status IdentifyMyself(grpc::ServerContext *context,
                                const IdentifyMyselfRequest *request,
                                IdentifyMyselfResponse *response) override;

    // RPC: The replica calls this and listens
    grpc::Status GetTransactions(
        grpc::ServerContext *context, const GetTransactionsRequest *request,
        grpc::ServerWriter<GetTransactionsResponse> *writer) override;

    std::deque<server::Address> cluster_addresses;
    std::shared_mutex cluster_addresses_mutex;

    std::set<grpc::ServerWriter<GetTransactionsResponse> *> cluster_writers;
    std::shared_mutex cluster_writers_mutex;

    std::set<grpc::ServerWriter<GetServersResponse> *> client_writers;
    std::shared_mutex client_writers_mutex;

    std::unique_ptr<Db> db;
};

}  // namespace link
}  // namespace service
}  // namespace converse
