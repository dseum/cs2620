#include <grpcpp/grpcpp.h>
#include <string>
#include <map>
#include <mutex>
#include <converse/service/link/link.grpc.pb.h>
#include "converse/service/link/link.pb.h"
#include "database.hpp"

namespace converse {
namespace service {
namespace link {

// Simple struct to hold info about each server
struct ServerInfo {
    bool id_assigned = false;
    uint64_t server_id = 0;  // 0 means unassigned
    std::string host;
    uint32_t port = 0;
};

// The gRPC service implementation for replication.
class LinkServiceImpl final : public LinkService::Service {
public:
    explicit LinkServiceImpl(Db* db);
    ~LinkServiceImpl() override;

    // RPC: Returns all known servers
    grpc::Status GetServers(::grpc::ServerContext* context,
                            const ::converse::service::link::GetServersRequest* request,
                            ::converse::service::link::GetServersResponse* response) override;

    // RPC: A new server calls this to broadcast its host/port. No ID assigned yet.
    grpc::Status IdentifyMyself(::grpc::ServerContext* context,
                                const ::converse::service::link::IdentifyMyselfRequest* request,
                                ::converse::service::link::IdentifyMyselfResponse* response) override;

    // RPC: After IdentifyMyself, the server calls this to claim a unique ID.
    grpc::Status ClaimServerId(::grpc::ServerContext* context,
                               const ::converse::service::link::ClaimServerIdRequest* request,
                               ::converse::service::link::ClaimServerIdResponse* response) override;

    // RPC: The leader calls this on each replica to replicate the same DB operations.
    grpc::Status ReplicateTransaction(::grpc::ServerContext* context,
                                      const ::converse::service::link::ReplicateTransactionRequest* request,
                                      ::converse::service::link::ReplicateTransactionResponse* response) override;
private:
    Db* db_; 
};

} // namespace link
} // namespace service
} // namespace converse
