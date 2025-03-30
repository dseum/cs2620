#pragma once

#include <converse/service/main/main.grpc.pb.h>
#include <grpcpp/grpcpp.h>

#include <condition_variable>
#include <mutex>

#include "converse/service/main/main.pb.h"

namespace converse {
namespace service {
namespace main {
namespace reader {
class ReceiveMessageReader

    : public grpc::ClientReadReactor<service::main::ReceiveMessageResponse> {
   public:
    ReceiveMessageReader(
        service::main::MainService::Stub *stub,
        const service::main::ReceiveMessageRequest &request,
        std::function<void(const grpc::Status &,
                           const service::main::ReceiveMessageResponse &)>
            on_response);
    void OnReadDone(bool ok) override;
    void OnDone(const grpc::Status &s) override;
    grpc::Status Await();
    void TryCancel();

   private:
    std::function<void(const grpc::Status &,
                       const service::main::ReceiveMessageResponse &)>
        on_response_;
    grpc::ClientContext context_;
    service::main::ReceiveMessageResponse response_;
    std::mutex mu_;
    std::condition_variable cv_;
    grpc::Status status_;
    bool done_ = false;
};

class ReceiveReadMessagesReader

    : public grpc::ClientReadReactor<
          service::main::ReceiveReadMessagesResponse> {
   public:
    ReceiveReadMessagesReader(
        service::main::MainService::Stub *stub,
        const service::main::ReceiveReadMessagesRequest &request,
        std::function<void(const grpc::Status &,
                           const service::main::ReceiveReadMessagesResponse &)>
            on_response);
    void OnReadDone(bool ok) override;
    void OnDone(const grpc::Status &s) override;
    grpc::Status Await();
    void TryCancel();

   private:
    std::function<void(const grpc::Status &,
                       const service::main::ReceiveReadMessagesResponse &)>
        on_response_;
    grpc::ClientContext context_;
    service::main::ReceiveReadMessagesResponse response_;
    std::mutex mu_;
    std::condition_variable cv_;
    grpc::Status status_;
    bool done_ = false;
};
}  // namespace reader
}  // namespace main
}  // namespace service
}  // namespace converse
