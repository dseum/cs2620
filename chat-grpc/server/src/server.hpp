#ifndef SERVER_HPP
#define SERVER_HPP

#include <memory>
#include <shared_mutex>

#include "database.hpp"
#include "gen/rpc/converse/converse.grpc.pb.h"

constexpr size_t HASHLEN = 32;
constexpr size_t SALTLEN = 16;

bool verify_password(const std::string& password,
                     const std::string& password_encoded);

std::string generate_password_encoded(
    const std::string& password,
    std::array<uint8_t, SALTLEN> salt = {0});

class MyConverseServiceImpl final : public converse::ConverseService::Service {
   public:
    MyConverseServiceImpl();
    ~MyConverseServiceImpl() override;
   
    void setDb(std::unique_ptr<Db> db) {
        db_ = std::move(db);
    }

    ::grpc::Status SignupUser(::grpc::ServerContext* context,
                              const converse::SignupUserRequest* request,
                              converse::SignupUserResponse* response) override;
    ::grpc::Status SigninUser(::grpc::ServerContext* context,
                              const converse::SigninUserRequest* request,
                              converse::SigninUserResponse* response) override;
    ::grpc::Status SignoutUser(
        ::grpc::ServerContext* context,
        const converse::SignoutUserRequest* request,
        converse::SignoutUserResponse* response) override;
    ::grpc::Status DeleteUser(::grpc::ServerContext* context,
                              const converse::DeleteUserRequest* request,
                              converse::DeleteUserResponse* response) override;
    ::grpc::Status GetOtherUsers(
        ::grpc::ServerContext* context,
        const converse::GetOtherUsersRequest* request,
        converse::GetOtherUsersResponse* response) override;
    ::grpc::Status CreateConversation(
        ::grpc::ServerContext* context,
        const converse::CreateConversationRequest* request,
        converse::CreateConversationResponse* response) override;
    ::grpc::Status GetConversations(
        ::grpc::ServerContext* context,
        const converse::GetConversationsRequest* request,
        converse::GetConversationsResponse* response) override;
    ::grpc::Status DeleteConversation(
        ::grpc::ServerContext* context,
        const converse::DeleteConversationRequest* request,
        converse::DeleteConversationResponse* response) override;
    ::grpc::Status SendMessage(
        ::grpc::ServerContext* context,
        const converse::SendMessageRequest* request,
        converse::SendMessageResponse* response) override;
    ::grpc::Status GetMessages(
        ::grpc::ServerContext* context,
        const converse::GetMessagesRequest* request,
        converse::GetMessagesResponse* response) override;
    ::grpc::Status DeleteMessage(
        ::grpc::ServerContext* context,
        const converse::DeleteMessageRequest* request,
        converse::DeleteMessageResponse* response) override;
    ::grpc::Status ReceiveMessage(
        ::grpc::ServerContext* context,
        const converse::ReceiveMessageRequest* request,
        ::grpc::ServerWriter<converse::MessageWithConversationId>* writer)
        override;

    std::unique_ptr<Db> db_;
    std::shared_mutex user_id_to_conversation_ids_mutex_;
    std::unordered_map<
        uint64_t, ::grpc::ServerWriter<converse::MessageWithConversationId>*>
        user_id_to_writer_;
};

#endif  // SERVER_HPP
