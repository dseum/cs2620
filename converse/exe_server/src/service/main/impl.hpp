#pragma once

#include <converse/service/link/link.pb.h>
#include <converse/service/main/main.grpc.pb.h>
#include <converse/service/main/main.pb.h>

#include <memory>
#include <shared_mutex>

#include "database.hpp"
#include "server.hpp"

constexpr size_t HASHLEN = 32;
constexpr size_t SALTLEN = 16;

bool verify_password(const std::string &password,
                     const std::string &password_encoded);

std::string generate_password_encoded(const std::string &password,
                                      std::array<uint8_t, SALTLEN> salt = {0});

namespace converse {
namespace service {
namespace main {
struct Writers {
    grpc::ServerWriter<ReceiveMessageResponse> *ReceiveMessage_writer;
    grpc::ServerWriter<ReceiveReadMessagesResponse> *ReceiveReadMessages_writer;
};

class Impl final : public MainService::Service {
   public:
    Impl(const std::string &name, const server::Address &address,
         std::set<grpc::ServerWriter<link::GetTransactionsResponse> *>
             &cluster_writers,
         std::shared_mutex &cluster_writers_mutex);
    ~Impl() override;

    grpc::Status SignupUser(grpc::ServerContext *context,
                            const SignupUserRequest *request,
                            SignupUserResponse *response) override;
    grpc::Status SigninUser(grpc::ServerContext *context,
                            const SigninUserRequest *request,
                            SigninUserResponse *response) override;
    grpc::Status SignoutUser(grpc::ServerContext *context,
                             const SignoutUserRequest *request,
                             SignoutUserResponse *response) override;
    grpc::Status DeleteUser(grpc::ServerContext *context,
                            const DeleteUserRequest *request,
                            DeleteUserResponse *response) override;
    grpc::Status GetOtherUsers(grpc::ServerContext *context,
                               const GetOtherUsersRequest *request,
                               GetOtherUsersResponse *response) override;
    grpc::Status CreateConversation(
        grpc::ServerContext *context, const CreateConversationRequest *request,
        CreateConversationResponse *response) override;
    grpc::Status GetConversation(grpc::ServerContext *context,
                                 const GetConversationRequest *request,
                                 GetConversationResponse *response) override;
    grpc::Status GetConversations(grpc::ServerContext *context,
                                  const GetConversationsRequest *request,
                                  GetConversationsResponse *response) override;
    grpc::Status DeleteConversation(
        grpc::ServerContext *context, const DeleteConversationRequest *request,
        DeleteConversationResponse *response) override;
    grpc::Status SendMessage(grpc::ServerContext *context,
                             const SendMessageRequest *request,
                             SendMessageResponse *response) override;
    grpc::Status GetMessages(grpc::ServerContext *context,
                             const GetMessagesRequest *request,
                             GetMessagesResponse *response) override;
    grpc::Status ReadMessages(grpc::ServerContext *context,
                              const ReadMessagesRequest *request,
                              ReadMessagesResponse *response) override;
    grpc::Status DeleteMessage(grpc::ServerContext *context,
                               const DeleteMessageRequest *request,
                               DeleteMessageResponse *response) override;
    grpc::Status ReceiveMessage(
        grpc::ServerContext *context, const ReceiveMessageRequest *request,
        grpc::ServerWriter<ReceiveMessageResponse> *writer) override;

    grpc::Status ReceiveReadMessages(
        grpc::ServerContext *context, const ReceiveReadMessagesRequest *request,
        grpc::ServerWriter<ReceiveReadMessagesResponse> *writer) override;

   private:
    std::string name_;
    converse::server::Address address_;
    std::unique_ptr<Db> db_;
    std::unordered_map<uint64_t, Writers> user_id_to_writers_;
    std::shared_mutex user_id_to_writers_mutex_;
    std::set<grpc::ServerWriter<link::GetTransactionsResponse> *>
        &cluster_writers_;
    std::shared_mutex &cluster_writers_mutex_;
};
}  // namespace main
}  // namespace service
}  // namespace converse
