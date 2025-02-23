#ifndef SERVER_HPP
#define SERVER_HPP

#include <memory>

#include "database.hpp"
#include "gen/rpc/converse/converse.grpc.pb.h"

class MyConverseServiceImpl final : public converse::ConverseService::Service {
   public:
    MyConverseServiceImpl();
    ~MyConverseServiceImpl() override;

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
    ::grpc::Status GetConversation(
        ::grpc::ServerContext* context,
        const converse::GetConversationRequest* request,
        converse::GetConversationResponse* response) override;
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

   private:
    std::unique_ptr<Db> db_;
};

#endif  // SERVER_HPP
