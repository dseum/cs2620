#pragma once

#include <converse/service/link/link.grpc.pb.h>
#include <converse/service/link/link.pb.h>
#include <grpcpp/grpcpp.h>

#include <condition_variable>
#include <mutex>

namespace converse {
namespace service {
namespace link {
namespace reader {
class GetTransactionsReader

    : public grpc::ClientReadReactor<GetTransactionsResponse> {
   public:
    GetTransactionsReader(LinkService::Stub *stub,
                          const GetTransactionsRequest &request,
                          std::function<void(const grpc::Status &,
                                             const GetTransactionsResponse &)>
                              on_response);
    void OnReadDone(bool ok) override;
    void OnDone(const grpc::Status &s) override;
    grpc::Status Await();
    void TryCancel();

   private:
    std::function<void(const grpc::Status &, const GetTransactionsResponse &)>
        on_response_;
    grpc::ClientContext context_;
    GetTransactionsResponse response_;
    std::mutex mu_;
    std::condition_variable cv_;
    grpc::Status status_;
    bool done_ = false;
};

}  // namespace reader
}  // namespace link
}  // namespace service
}  // namespace converse
