#include "reader.hpp"

namespace converse {
namespace service {
namespace link {
namespace reader {

GetServersReader::GetServersReader(
    LinkService::Stub *stub, const GetServersRequest &request,
    std::function<void(const grpc::Status &, const GetServersResponse &)>
        on_response,
    std::function<void(const grpc::Status &)> on_done)
    : on_response_(on_response), on_done_(on_done) {
    stub->async()->GetServers(&context_, &request, this);
    StartRead(&response_);
    StartCall();
}

void GetServersReader::OnReadDone(bool ok) {
    if (ok) {
        on_response_(grpc::Status::OK, response_);
        response_.Clear();
        response_ = GetServersResponse();
        StartRead(&response_);
    }
}

void GetServersReader::OnDone(const grpc::Status &s) {
    std::unique_lock l(mu_);
    status_ = s;
    done_ = true;
    cv_.notify_one();
    on_done_(s);
}

grpc::Status GetServersReader::Await() {
    std::unique_lock l(mu_);
    cv_.wait(l, [this] { return done_; });
    return std::move(status_);
}

void GetServersReader::TryCancel() { context_.TryCancel(); }
}  // namespace reader
}  // namespace link

namespace main {
namespace reader {

ReceiveMessageReader::ReceiveMessageReader(
    MainService::Stub *stub, const ReceiveMessageRequest &request,
    std::function<void(const grpc::Status &, const ReceiveMessageResponse &)>
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
    std::unique_lock l(mu_);
    status_ = s;
    done_ = true;
    cv_.notify_one();
}

grpc::Status ReceiveMessageReader::Await() {
    std::unique_lock l(mu_);
    cv_.wait(l, [this] { return done_; });
    return std::move(status_);
}

void ReceiveMessageReader::TryCancel() { context_.TryCancel(); }

ReceiveReadMessagesReader::ReceiveReadMessagesReader(
    MainService::Stub *stub, const ReceiveReadMessagesRequest &request,
    std::function<void(const grpc::Status &,
                       const ReceiveReadMessagesResponse &)>
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
    std::unique_lock l(mu_);
    status_ = s;
    done_ = true;
    cv_.notify_one();
}

grpc::Status ReceiveReadMessagesReader::Await() {
    std::unique_lock l(mu_);
    cv_.wait(l, [this] { return done_; });
    return std::move(status_);
}

void ReceiveReadMessagesReader::TryCancel() { context_.TryCancel(); }

}  // namespace reader
}  // namespace main
}  // namespace service
}  // namespace converse
