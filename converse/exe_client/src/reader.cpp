#include "reader.hpp"

namespace converse {
namespace service {
namespace main {
namespace reader {

ReceiveMessageReader::ReceiveMessageReader(
    service::main::MainService::Stub *stub,
    const service::main::ReceiveMessageRequest &request,
    std::function<void(const grpc::Status &,
                       const service::main::ReceiveMessageResponse &)>
        on_response)
    : on_response_(on_response) {
    stub->async()->ReceiveMessage(&context_, &request, this);
    StartRead(&response_);
    StartCall();
}

void ReceiveMessageReader::OnReadDone(bool ok) {
    if (ok) {
        on_response_(grpc::Status::OK, response_);
        StartRead(&response_);
    }
}

void ReceiveMessageReader::OnDone(const grpc::Status &s) {
    std::unique_lock<std::mutex> l(mu_);
    status_ = s;
    done_ = true;
    cv_.notify_one();
}

grpc::Status ReceiveMessageReader::Await() {
    std::unique_lock<std::mutex> l(mu_);
    cv_.wait(l, [this] { return done_; });
    return std::move(status_);
}

void ReceiveMessageReader::TryCancel() { context_.TryCancel(); }

ReceiveReadMessagesReader::ReceiveReadMessagesReader(
    service::main::MainService::Stub *stub,
    const service::main::ReceiveReadMessagesRequest &request,
    std::function<void(const grpc::Status &,
                       const service::main::ReceiveReadMessagesResponse &)>
        on_response)
    : on_response_(on_response) {
    stub->async()->ReceiveReadMessages(&context_, &request, this);
    StartRead(&response_);
    StartCall();
}

void ReceiveReadMessagesReader::OnReadDone(bool ok) {
    if (ok) {
        on_response_(grpc::Status::OK, response_);
        StartRead(&response_);
    }
}

void ReceiveReadMessagesReader::OnDone(const grpc::Status &s) {
    std::unique_lock<std::mutex> l(mu_);
    status_ = s;
    done_ = true;
    cv_.notify_one();
}

grpc::Status ReceiveReadMessagesReader::Await() {
    std::unique_lock<std::mutex> l(mu_);
    cv_.wait(l, [this] { return done_; });
    return std::move(status_);
}

void ReceiveReadMessagesReader::TryCancel() { context_.TryCancel(); }

}  // namespace reader
}  // namespace main
}  // namespace service
}  // namespace converse
