#pragma once

#include <converse/service/link/link.grpc.pb.h>
#include <converse/service/link/link.pb.h>
#include <converse/service/main/main.grpc.pb.h>
#include <converse/service/main/main.pb.h>
#include <grpcpp/grpcpp.h>

#include <condition_variable>
#include <mutex>

namespace converse {
namespace service {
namespace link {
namespace reader {

class GetServersReader

    : public grpc::ClientReadReactor<GetServersResponse> {
   public:
    GetServersReader(
        LinkService::Stub *stub, const GetServersRequest &request,
        std::function<void(const grpc::Status &, const GetServersResponse &)>
            on_response,
        std::function<void(const grpc::Status &)> on_done);
    void OnReadDone(bool ok) override;
    void OnDone(const grpc::Status &s) override;
    grpc::Status Await();
    void TryCancel();

   private:
    std::function<void(const grpc::Status &, const GetServersResponse &)>
        on_response_;
    std::function<void(const grpc::Status &)> on_done_;
    grpc::ClientContext context_;
    GetServersResponse response_;
    std::mutex mu_;
    std::condition_variable cv_;
    grpc::Status status_;
    bool done_ = false;
};
}  // namespace reader
}  // namespace link

namespace main {
namespace reader {
class ReceiveMessageReader

    : public grpc::ClientReadReactor<ReceiveMessageResponse> {
   public:
    ReceiveMessageReader(MainService::Stub *stub,
                         const ReceiveMessageRequest &request,
                         std::function<void(const grpc::Status &,
                                            const ReceiveMessageResponse &)>
                             on_response);
    void OnReadDone(bool ok) override;
    void OnDone(const grpc::Status &s) override;
    grpc::Status Await();
    void TryCancel();

   private:
    std::function<void(const grpc::Status &, const ReceiveMessageResponse &)>
        on_response_;
    grpc::ClientContext context_;
    ReceiveMessageResponse response_;
    std::mutex mu_;
    std::condition_variable cv_;
    grpc::Status status_;
    bool done_ = false;
};

class ReceiveReadMessagesReader

    : public grpc::ClientReadReactor<ReceiveReadMessagesResponse> {
   public:
    ReceiveReadMessagesReader(
        MainService::Stub *stub, const ReceiveReadMessagesRequest &request,
        std::function<void(const grpc::Status &,
                           const ReceiveReadMessagesResponse &)>
            on_response);
    void OnReadDone(bool ok) override;
    void OnDone(const grpc::Status &s) override;
    grpc::Status Await();
    void TryCancel();

   private:
    std::function<void(const grpc::Status &,
                       const ReceiveReadMessagesResponse &)>
        on_response_;
    grpc::ClientContext context_;
    ReceiveReadMessagesResponse response_;
    std::mutex mu_;
    std::condition_variable cv_;
    grpc::Status status_;
    bool done_ = false;
};
}  // namespace reader
}  // namespace main
}  // namespace service
}  // namespace converse
