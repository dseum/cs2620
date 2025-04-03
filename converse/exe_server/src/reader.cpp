#include "reader.hpp"

namespace converse {
namespace service {
namespace link {
namespace reader {

GetTransactionsReader::GetTransactionsReader(
    LinkService::Stub *stub, const GetTransactionsRequest &request,
    std::function<void(const grpc::Status &, const GetTransactionsResponse &)>
        on_response)
    : on_response_(on_response) {
    stub->async()->GetTransactions(&context_, &request, this);
    StartRead(&response_);
    StartCall();
}

void GetTransactionsReader::OnReadDone(bool ok) {
    if (ok) {
        on_response_(grpc::Status::OK, response_);
        StartRead(&response_);
    }
}

void GetTransactionsReader::OnDone(const grpc::Status &s) {
    std::unique_lock l(mu_);
    status_ = s;
    done_ = true;
    cv_.notify_one();
}

grpc::Status GetTransactionsReader::Await() {
    std::unique_lock l(mu_);
    cv_.wait(l, [this] { return done_; });
    return std::move(status_);
}

void GetTransactionsReader::TryCancel() { context_.TryCancel(); }

}  // namespace reader
}  // namespace link
}  // namespace service
}  // namespace converse
