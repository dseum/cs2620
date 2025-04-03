#include "service/link/impl.hpp"

#include <converse/service/link/link.pb.h>
#include <grpcpp/grpcpp.h>

#include <converse/logging/core.hpp>
#include <mutex>
#include <string>

#include "database.hpp"

namespace lg = converse::logging;

namespace converse {
namespace service {
namespace link {

Impl::Impl(const std::string &name, const server::Address &address)
    : db(std::make_unique<Db>(name)) {}

Impl::~Impl() = default;

grpc::Status Impl::GetServers(grpc::ServerContext *context,
                              const GetServersRequest *request,
                              grpc::ServerWriter<GetServersResponse> *writer) {
    lg::write(lg::level::info, "GetServers()");
    std::unique_lock lock(
        client_writers_mutex);  // needs to be here to prevent data race with
                                // IdentifyMyself
    {
        std::shared_lock l(cluster_addresses_mutex);
        GetServersResponse response;
        for (const auto &addr : cluster_addresses) {
            auto address = response.add_cluster_addresses();
            address->set_host(addr.host);
            address->set_port(addr.port);
        }
        writer->Write(response);
    }
    auto [it, inserted] = client_writers.emplace(writer);
    lock.unlock();
    lg::write(lg::level::info, "GetServers() -> +");
    if (!inserted) {
        lg::write(lg::level::info,
                  "GetServers() -> already exists, but this should never "
                  "happen");
        return grpc::Status(grpc::StatusCode::ALREADY_EXISTS,
                            "already exists, but this should never happen");
    }
    while (!context->IsCancelled()) {
    }
    lock.lock();
    client_writers.erase(it);
    lock.unlock();
    lg::write(lg::level::info, "GetServers() -> -");
    return grpc::Status::OK;
}

grpc::Status Impl::IdentifyMyself(grpc::ServerContext *context,
                                  const IdentifyMyselfRequest *request,
                                  IdentifyMyselfResponse *response) {
    std::string host = request->host();
    uint32_t port = request->port();
    {
        std::unique_lock lock(cluster_addresses_mutex);
        cluster_addresses.emplace_back(host, port);
        for (const auto &addr : cluster_addresses) {
            auto *address = response->add_cluster_addresses();
            address->set_host(addr.host);
            address->set_port(addr.port);
        }
    }
    lg::write(lg::level::info, "IdentifyMyself({}:{})", host, port);
    {
        std::shared_lock lock(client_writers_mutex);
        GetServersResponse response;
        auto address = response.add_cluster_addresses();
        address->set_host(host);
        address->set_port(port);
        for (auto *writer : client_writers) {
            bool status = writer->Write(response);
            lg::write(lg::level::info, "IdentifyMyself() -> {}",
                      status ? "true" : "false");
        }
    }
    response->set_database(db->get_bytes());
    return grpc::Status::OK;
}

grpc::Status Impl::GetTransactions(
    grpc::ServerContext *context, const GetTransactionsRequest *request,
    grpc::ServerWriter<GetTransactionsResponse> *writer) {
    lg::write(lg::level::info, "GetTransactions()");
    std::unique_lock lock(cluster_writers_mutex);
    auto [it, inserted] = cluster_writers.emplace(writer);
    lg::write(lg::level::info, "GetTransactions() -> +");
    lock.unlock();
    if (!inserted) {
        lg::write(lg::level::info,
                  "GetTransactions() -> already exists, but this should never "
                  "happen");
        return grpc::Status(grpc::StatusCode::ALREADY_EXISTS,
                            "already exists, but this should never happen");
    }
    while (!context->IsCancelled()) {
    }
    lock.lock();
    cluster_writers.erase(it);
    lock.unlock();
    lg::write(lg::level::info, "GetTransactions() -> -");
    return grpc::Status::CANCELLED;
}

}  // namespace link
}  // namespace service
}  // namespace converse
