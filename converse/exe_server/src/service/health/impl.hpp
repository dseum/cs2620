#pragma once

#include <converse/service/health/health.grpc.pb.h>
#include <converse/service/health/health.pb.h>
#include <grpcpp/grpcpp.h>

namespace converse {
namespace service {
namespace health {

// The gRPC service implementation for replication.
class Impl final : public HealthService::Service {
   public:
    explicit Impl();
    ~Impl() override;

    grpc::Status HealthCheck(grpc::ServerContext *context,
                             const HealthCheckRequest *request,
                             HealthCheckResponse *response) override;
};

}  // namespace health
}  // namespace service
}  // namespace converse
