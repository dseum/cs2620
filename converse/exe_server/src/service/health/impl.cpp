#include "service/health/impl.hpp"

#include <converse/service/health/health.pb.h>
#include <grpcpp/grpcpp.h>

#include <converse/logging/core.hpp>

namespace lg = converse::logging;

namespace converse {
namespace service {
namespace health {

Impl::Impl() {}

Impl::~Impl() = default;

grpc::Status Impl::HealthCheck(grpc::ServerContext *context,
                               const HealthCheckRequest *request,
                               HealthCheckResponse *response) {
    lg::write(lg::level::info, "HealthCheck");
    return grpc::Status::OK;
}
}  // namespace health
}  // namespace service
}  // namespace converse
