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
            on_response)
        : on_response_(on_response) {
        stub->async()->ReceiveMessage(&context_, &request, this);
        StartRead(&response_);
        StartCall();
    }

    void OnReadDone(bool ok) override {
        if (ok) {
            on_response_(grpc::Status::OK, response_);
            StartRead(&response_);
        }
    }

    void OnDone(const grpc::Status &s) override {
        std::unique_lock<std::mutex> l(mu_);
        status_ = s;
        done_ = true;
        cv_.notify_one();
    }

    grpc::Status Await() {
        std::unique_lock<std::mutex> l(mu_);
        cv_.wait(l, [this] { return done_; });
        return std::move(status_);
    }

    void TryCancel() { context_.TryCancel(); }

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
            on_response)
        : on_response_(on_response) {
        stub->async()->ReceiveReadMessages(&context_, &request, this);
        StartRead(&response_);
        StartCall();
    }

    void OnReadDone(bool ok) override {
        if (ok) {
            on_response_(grpc::Status::OK, response_);
            StartRead(&response_);
        }
    }

    void OnDone(const grpc::Status &s) override {
        std::unique_lock<std::mutex> l(mu_);
        status_ = s;
        done_ = true;
        cv_.notify_one();
    }

    grpc::Status Await() {
        std::unique_lock<std::mutex> l(mu_);
        cv_.wait(l, [this] { return done_; });
        return std::move(status_);
    }

    void TryCancel() { context_.TryCancel(); }

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
