#include "client.hpp"

#include <print>

#include "../utils.hpp"

ClientRequest::ClientRequest(OperationCode operation_code)
    : operation_code(operation_code) {}

ClientResponse::ClientResponse(OperationCode operation_code,
                               StatusCode status_code)
    : operation_code(operation_code), status_code(status_code) {}

Data ClientResponse::serialize() const {
    size_t size = OperationCodeWrapper::size() + StatusCodeWrapper::size();
    Data data = initialize_serialization(size);
    OperationCodeWrapper::serialize_to(data, operation_code);
    StatusCodeWrapper::serialize_to(data, status_code);
    return data;
}

ClientSignupUserRequest::ClientSignupUserRequest(Data& data,
                                                 Data::size_type offset)
    : ClientRequest(OperationCode::CLIENT_SIGNUP_USER) {
    auto [username_, offset1] = StringWrapper::deserialize_from(data, offset);
    auto [password_, offset2] = StringWrapper::deserialize_from(data, offset1);
    username = username_;
    password = password_;
}

ClientSignupUserResponse::ClientSignupUserResponse(StatusCode status_code,
                                                   Id user_id)
    : ClientResponse(OperationCode::CLIENT_SIGNUP_USER, status_code),
      user_id(user_id) {}

Data ClientSignupUserResponse::serialize() const {
    size_t size = OperationCodeWrapper::size() + StatusCodeWrapper::size() +
                  IdWrapper::size();
    Data data = initialize_serialization(size);
    OperationCodeWrapper::serialize_to(data, operation_code);
    StatusCodeWrapper::serialize_to(data, status_code);
    IdWrapper::serialize_to(data, user_id);
    return data;
}

ClientSigninUserRequest::ClientSigninUserRequest(Data& data,
                                                 Data::size_type offset)
    : ClientRequest(OperationCode::CLIENT_SIGNIN_USER) {
    auto [username_, offset1] = StringWrapper::deserialize_from(data, offset);
    auto [password_, offset2] = StringWrapper::deserialize_from(data, offset1);
    username = username_;
    password = password_;
}

ClientSigninUserResponse::ClientSigninUserResponse(StatusCode status_code,
                                                   Id user_id)
    : ClientResponse(OperationCode::CLIENT_SIGNIN_USER, status_code),
      user_id(user_id) {}

Data ClientSigninUserResponse::serialize() const {
    size_t size = OperationCodeWrapper::size() + StatusCodeWrapper::size() +
                  IdWrapper::size();
    Data data = initialize_serialization(size);
    OperationCodeWrapper::serialize_to(data, operation_code);
    StatusCodeWrapper::serialize_to(data, status_code);
    IdWrapper::serialize_to(data, user_id);
    return data;
}

ClientSignoutUserRequest::ClientSignoutUserRequest(Data& data,
                                                   Data::size_type offset)
    : ClientRequest(OperationCode::CLIENT_SIGNOUT_USER) {
    auto [user_id_, offset1] = IdWrapper::deserialize_from(data, offset);
    user_id = user_id_;
}

ClientSignoutUserResponse::ClientSignoutUserResponse(StatusCode status_code)
    : ClientResponse(OperationCode::CLIENT_SIGNOUT_USER, status_code) {}

ClientDeleteUserRequest::ClientDeleteUserRequest(Data& data,
                                                 Data::size_type offset)
    : ClientRequest(OperationCode::CLIENT_DELETE_USER) {
    auto [user_id_, offset1] = IdWrapper::deserialize_from(data, offset);
    auto [password_, offset2] = StringWrapper::deserialize_from(data, offset1);
    user_id = user_id_;
    password = password_;
}

ClientDeleteUserResponse::ClientDeleteUserResponse(StatusCode status_code)
    : ClientResponse(OperationCode::CLIENT_DELETE_USER, status_code) {}

ClientGetOtherUsersRequest::ClientGetOtherUsersRequest(Data& data,
                                                       Data::size_type offset)
    : ClientRequest(OperationCode::CLIENT_GET_OTHER_USERS) {
    std::tie(user_id, offset) = IdWrapper::deserialize_from(data, offset);
    std::tie(query, offset) = StringWrapper::deserialize_from(data, offset);
    std::tie(limit, offset) = SizeWrapper::deserialize_from(data, offset);
    std::tie(this->offset, offset) =
        SizeWrapper::deserialize_from(data, offset);
}

ClientGetOtherUsersResponse::ClientGetOtherUsersResponse(
    StatusCode status_code, std::vector<std::tuple<Id, String>> users)
    : ClientResponse(OperationCode::CLIENT_GET_OTHER_USERS, status_code),
      users(users) {}

Data ClientGetOtherUsersResponse::serialize() const {
    size_t size = OperationCodeWrapper::size() + StatusCodeWrapper::size() +
                  SizeWrapper::size();
    for (const auto& [id, username] : users) {
        size += IdWrapper::size() + StringWrapper::size(username);
    }
    Data data = initialize_serialization(size);
    OperationCodeWrapper::serialize_to(data, operation_code);
    StatusCodeWrapper::serialize_to(data, status_code);
    SizeWrapper::serialize_to(data, users.size());
    for (const auto& [id, username] : users) {
        IdWrapper::serialize_to(data, id);
        StringWrapper::serialize_to(data, username);
    }
    return data;
}

ClientCreateConversationRequest::ClientCreateConversationRequest(
    Data& data, Data::size_type offset)
    : ClientRequest(OperationCode::CLIENT_CREATE_CONVERSATION) {
    auto [send_user_id_, offset1] = IdWrapper::deserialize_from(data, offset);
    auto [recv_user_id_, offset2] = IdWrapper::deserialize_from(data, offset1);
    send_user_id = send_user_id_;
    recv_user_id = recv_user_id_;
}

ClientCreateConversationResponse::ClientCreateConversationResponse(
    StatusCode status_code, Id conversation_id, Id recv_user_id,
    String& recv_user_username)
    : ClientResponse(OperationCode::CLIENT_CREATE_CONVERSATION, status_code),
      conversation_id(conversation_id),
      recv_user_id(recv_user_id),
      recv_user_username(recv_user_username) {}

Data ClientCreateConversationResponse::serialize() const {
    size_t size = OperationCodeWrapper::size() + StatusCodeWrapper::size() +
                  IdWrapper::size() + IdWrapper::size() +
                  StringWrapper::size(recv_user_username);
    Data data = initialize_serialization(size);
    OperationCodeWrapper::serialize_to(data, operation_code);
    StatusCodeWrapper::serialize_to(data, status_code);
    IdWrapper::serialize_to(data, conversation_id);
    IdWrapper::serialize_to(data, recv_user_id);
    StringWrapper::serialize_to(data, recv_user_username);
    return data;
}

ClientGetConversationsRequest::ClientGetConversationsRequest(
    Data& data, Data::size_type offset)
    : ClientRequest(OperationCode::CLIENT_GET_CONVERSATIONS) {
    auto [user_id_, offset1] = IdWrapper::deserialize_from(data, offset);
    user_id = user_id_;
}

ClientGetConversationsResponse::ClientGetConversationsResponse(
    StatusCode status_code,
    std::vector<std::tuple<Id, Id, String, Size>> conversations)
    : ClientResponse(OperationCode::CLIENT_GET_CONVERSATIONS, status_code),
      conversations(conversations) {}

Data ClientGetConversationsResponse::serialize() const {
    size_t size = OperationCodeWrapper::size() + StatusCodeWrapper::size() +
                  SizeWrapper::size();
    for (const auto& [id, send_user_id, recv_user_id, message_count] :
         conversations) {
        size += IdWrapper::size() + IdWrapper::size() +
                StringWrapper::size(recv_user_id) + SizeWrapper::size();
    }
    Data data = initialize_serialization(size);
    OperationCodeWrapper::serialize_to(data, operation_code);
    StatusCodeWrapper::serialize_to(data, status_code);
    SizeWrapper::serialize_to(data, conversations.size());
    for (const auto& [id, send_user_id, recv_user_id, message_count] :
         conversations) {
        IdWrapper::serialize_to(data, id);
        IdWrapper::serialize_to(data, send_user_id);
        StringWrapper::serialize_to(data, recv_user_id);
        SizeWrapper::serialize_to(data, message_count);
    }
    std::println("serialized get conversations response {}", data.size());
    return data;
}

ClientDeleteConversationRequest::ClientDeleteConversationRequest(
    Data& data, Data::size_type offset)
    : ClientRequest(OperationCode::CLIENT_DELETE_CONVERSATION) {
    auto [conversation_id_, offset1] =
        IdWrapper::deserialize_from(data, offset);
    conversation_id = conversation_id_;
}

ClientDeleteConversationResponse::ClientDeleteConversationResponse(
    StatusCode status_code)
    : ClientResponse(OperationCode::CLIENT_DELETE_CONVERSATION, status_code) {}

ClientSendMessageRequest::ClientSendMessageRequest(Data& data,
                                                   Data::size_type offset)
    : ClientRequest(OperationCode::CLIENT_SEND_MESSAGE) {
    std::tie(conversation_id, offset) =
        IdWrapper::deserialize_from(data, offset);
    std::tie(send_user_id, offset) = IdWrapper::deserialize_from(data, offset);
    std::tie(content, offset) = StringWrapper::deserialize_from(data, offset);
}

ClientSendMessageResponse::ClientSendMessageResponse(StatusCode status_code,
                                                     Id message_id)
    : ClientResponse(OperationCode::CLIENT_SEND_MESSAGE, status_code),
      message_id(message_id) {}

Data ClientSendMessageResponse::serialize() const {
    size_t size = OperationCodeWrapper::size() + StatusCodeWrapper::size() +
                  IdWrapper::size();
    Data data = initialize_serialization(size);
    OperationCodeWrapper::serialize_to(data, operation_code);
    StatusCodeWrapper::serialize_to(data, status_code);
    IdWrapper::serialize_to(data, message_id);
    return data;
}

ClientGetMessagesRequest::ClientGetMessagesRequest(Data& data,
                                                   Data::size_type offset)
    : ClientRequest(OperationCode::CLIENT_GET_MESSAGES) {
    std::tie(conversation_id, offset) =
        IdWrapper::deserialize_from(data, offset);
}

ClientGetMessagesResponse::ClientGetMessagesResponse(
    StatusCode status_code,
    std::vector<std::tuple<Id, Id, Bool, String>> messages)
    : ClientResponse(OperationCode::CLIENT_GET_MESSAGES, status_code),
      messages(messages) {}

Data ClientGetMessagesResponse::serialize() const {
    size_t size = OperationCodeWrapper::size() + StatusCodeWrapper::size() +
                  SizeWrapper::size();
    for (const auto& [id, send_user_id, is_read, content] : messages) {
        size += IdWrapper::size() + IdWrapper::size() + BoolWrapper::size() +
                StringWrapper::size(content);
    }
    Data data = initialize_serialization(size);
    OperationCodeWrapper::serialize_to(data, operation_code);
    StatusCodeWrapper::serialize_to(data, status_code);
    SizeWrapper::serialize_to(data, messages.size());
    for (const auto& [id, send_user_id, is_read, content] : messages) {
        IdWrapper::serialize_to(data, id);
        IdWrapper::serialize_to(data, send_user_id);
        BoolWrapper::serialize_to(data, is_read);
        StringWrapper::serialize_to(data, content);
    }
    return data;
}

ClientDeleteMessageRequest::ClientDeleteMessageRequest(Data& data,
                                                       Data::size_type offset)
    : ClientRequest(OperationCode::CLIENT_DELETE_MESSAGE) {
    std::tie(conversation_id, offset) =
        IdWrapper::deserialize_from(data, offset);
    std::tie(message_id, offset) = IdWrapper::deserialize_from(data, offset);
}

ClientDeleteMessageResponse::ClientDeleteMessageResponse(StatusCode status_code)
    : ClientResponse(OperationCode::CLIENT_DELETE_MESSAGE, status_code) {}
