#pragma once

#include <boost/json.hpp>

#include "utils.hpp"

class ClientRequest {
   public:
    OperationCode operation_code;

    ClientRequest(OperationCode operation_code);
};

class ClientResponse {
   public:
    OperationCode operation_code;
    StatusCode status_code;

    ClientResponse(OperationCode operation_code, StatusCode status_code);
    virtual ~ClientResponse() = default;

    virtual Data serialize() const;
};

class ClientSignupUserRequest : public ClientRequest {
   public:
    String username;
    String password;

    ClientSignupUserRequest(boost::json::object &obj);
};

class ClientSignupUserResponse : public ClientResponse {
   public:
    Id user_id;

    ClientSignupUserResponse(StatusCode status_code, Id user_id);
    Data serialize() const override;
};

class ClientSigninUserRequest : public ClientRequest {
   public:
    String username;
    String password;

    ClientSigninUserRequest(boost::json::object &obj);
};

class ClientSigninUserResponse : public ClientResponse {
   public:
    Id user_id;

    ClientSigninUserResponse(StatusCode status_code, Id user_id);
    Data serialize() const override;
};

class ClientSignoutUserRequest : public ClientRequest {
   public:
    Id user_id;

    ClientSignoutUserRequest(boost::json::object &obj);
};

class ClientSignoutUserResponse : public ClientResponse {
   public:
    ClientSignoutUserResponse(StatusCode status_code);
};

class ClientDeleteUserRequest : public ClientRequest {
   public:
    Id user_id;
    String password;

    ClientDeleteUserRequest(boost::json::object &obj);
};

class ClientDeleteUserResponse : public ClientResponse {
   public:
    ClientDeleteUserResponse(StatusCode status_code);
};

class ClientGetOtherUsersRequest : public ClientRequest {
   public:
    Id user_id;
    String query;
    Size limit;
    Size offset;

    ClientGetOtherUsersRequest(boost::json::object &obj);
};

class ClientGetOtherUsersResponse : public ClientResponse {
   public:
    std::vector<std::tuple<Id, String>> users;

    ClientGetOtherUsersResponse(StatusCode status_code,
                                std::vector<std::tuple<Id, String>> users);
    Data serialize() const override;
};

class ClientCreateConversationRequest : public ClientRequest {
   public:
    Id send_user_id;
    Id recv_user_id;

    ClientCreateConversationRequest(boost::json::object &obj);
};

class ClientCreateConversationResponse : public ClientResponse {
   public:
    Id conversation_id;
    Id recv_user_id;
    String recv_user_username;

    ClientCreateConversationResponse(StatusCode status_code, Id conversation_id,
                                     Id recv_user_id,
                                     String &recv_user_username);
    Data serialize() const override;
};

class ClientGetConversationsRequest : public ClientRequest {
   public:
    Id user_id;

    ClientGetConversationsRequest(boost::json::object &obj);
};

class ClientGetConversationsResponse : public ClientResponse {
   public:
    std::vector<std::tuple<Id, Id, String, Size>> conversations;

    ClientGetConversationsResponse(
        StatusCode status_code,
        std::vector<std::tuple<Id, Id, String, Size>> conversations);
    Data serialize() const override;
};

class ClientDeleteConversationRequest : public ClientRequest {
   public:
    Id conversation_id;

    ClientDeleteConversationRequest(boost::json::object &obj);
};

class ClientDeleteConversationResponse : public ClientResponse {
   public:
    ClientDeleteConversationResponse(StatusCode status_code);
};

class ClientSendMessageRequest : public ClientRequest {
   public:
    Id conversation_id;
    Id send_user_id;
    String content;

    ClientSendMessageRequest(boost::json::object &obj);
};

class ClientSendMessageResponse : public ClientResponse {
   public:
    Id message_id;

    ClientSendMessageResponse(StatusCode status_code, Id message_id);
    Data serialize() const override;
};

class ClientGetMessagesRequest : public ClientRequest {
   public:
    Id conversation_id;

    ClientGetMessagesRequest(boost::json::object &obj);
};

class ClientGetMessagesResponse : public ClientResponse {
   public:
    std::vector<std::tuple<Id, Id, Bool, String>> messages;

    ClientGetMessagesResponse(
        StatusCode status_code,
        std::vector<std::tuple<Id, Id, Bool, String>> messages);
    Data serialize() const override;
};

class ClientDeleteMessageRequest : public ClientRequest {
   public:
    Id conversation_id;
    Id message_id;

    ClientDeleteMessageRequest(boost::json::object &obj);
};

class ClientDeleteMessageResponse : public ClientResponse {
   public:
    ClientDeleteMessageResponse(StatusCode status_code);
};
