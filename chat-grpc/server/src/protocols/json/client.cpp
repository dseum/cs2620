#include "client.hpp"

#include <boost/json.hpp>
#include <print>

#include "../utils.hpp"

ClientRequest::ClientRequest(OperationCode operation_code)
    : operation_code(operation_code) {}

ClientResponse::ClientResponse(OperationCode operation_code,
                               StatusCode status_code)
    : operation_code(operation_code), status_code(status_code) {}

Data ClientResponse::serialize() const {
    boost::json::object obj = {
        {"operation_code", static_cast<int>(operation_code)},
        {"status_code", static_cast<int>(status_code)}};
    std::string res = boost::json::serialize(obj);
    Data data = initialize_serialization(res.size());
    data.insert(data.end(), res.begin(), res.end());
    return data;
}

ClientSignupUserRequest::ClientSignupUserRequest(boost::json::object& obj)
    : ClientRequest(OperationCode::CLIENT_SIGNUP_USER) {
    username = boost::json::value_to<String>(obj.at("username"));
    password = boost::json::value_to<String>(obj.at("password"));
}

ClientSignupUserResponse::ClientSignupUserResponse(StatusCode status_code,
                                                   Id user_id)
    : ClientResponse(OperationCode::CLIENT_SIGNUP_USER, status_code),
      user_id(user_id) {
    printf("ClientSignupUserResponse constructor called\n");
}

Data ClientSignupUserResponse::serialize() const {
    boost::json::object obj = {
        {"operation_code", static_cast<int>(operation_code)},
        {"status_code", static_cast<int>(status_code)},
        {"user_id", user_id}};
    std::string res = boost::json::serialize(obj);
    Data data = initialize_serialization(res.size());
    data.insert(data.end(), res.begin(), res.end());
    return data;
}

ClientSigninUserRequest::ClientSigninUserRequest(boost::json::object& obj)
    : ClientRequest(OperationCode::CLIENT_SIGNIN_USER) {
    username = boost::json::value_to<String>(obj.at("username"));
    password = boost::json::value_to<String>(obj.at("password"));
}

ClientSigninUserResponse::ClientSigninUserResponse(StatusCode status_code,
                                                   Id user_id)
    : ClientResponse(OperationCode::CLIENT_SIGNIN_USER, status_code),
      user_id(user_id) {}

Data ClientSigninUserResponse::serialize() const {
    boost::json::object obj = {
        {"operation_code", static_cast<int>(operation_code)},
        {"status_code", static_cast<int>(status_code)},
        {"user_id", user_id}};
    std::string res = boost::json::serialize(obj);
    Data data = initialize_serialization(res.size());
    data.insert(data.end(), res.begin(), res.end());
    return data;
}

ClientSignoutUserRequest::ClientSignoutUserRequest(boost::json::object& obj)
    : ClientRequest(OperationCode::CLIENT_SIGNOUT_USER) {
    user_id = boost::json::value_to<Id>(obj.at("user_id"));
}

ClientSignoutUserResponse::ClientSignoutUserResponse(StatusCode status_code)
    : ClientResponse(OperationCode::CLIENT_SIGNOUT_USER, status_code) {}

ClientDeleteUserRequest::ClientDeleteUserRequest(boost::json::object& obj)
    : ClientRequest(OperationCode::CLIENT_DELETE_USER) {
    user_id = boost::json::value_to<Id>(obj.at("user_id"));
    password = boost::json::value_to<String>(obj.at("password"));
}

ClientDeleteUserResponse::ClientDeleteUserResponse(StatusCode status_code)
    : ClientResponse(OperationCode::CLIENT_DELETE_USER, status_code) {}

ClientGetOtherUsersRequest::ClientGetOtherUsersRequest(boost::json::object& obj)
    : ClientRequest(OperationCode::CLIENT_GET_OTHER_USERS) {
    user_id = boost::json::value_to<Id>(obj.at("user_id"));
    query = boost::json::value_to<String>(obj.at("query"));
    limit = boost::json::value_to<Size>(obj.at("limit"));
    offset = boost::json::value_to<Size>(obj.at("offset"));
}

ClientGetOtherUsersResponse::ClientGetOtherUsersResponse(
    StatusCode status_code, std::vector<std::tuple<Id, String>> users)
    : ClientResponse(OperationCode::CLIENT_GET_OTHER_USERS, status_code),
      users(users) {}

Data ClientGetOtherUsersResponse::serialize() const {
    boost::json::object obj = {
        {"operation_code", static_cast<int>(operation_code)},
        {"status_code", static_cast<int>(status_code)},
    };
    boost::json::array users_array;
    for (const auto& [id, username] : users) {
        boost::json::object user_obj = {
            {"id", id},
            {"username", username},
        };
        users_array.emplace_back(user_obj);
    }
    obj["users"] = users_array;
    std::string res = boost::json::serialize(obj);
    Data data = initialize_serialization(res.size());
    data.insert(data.end(), res.begin(), res.end());
    return data;
}

ClientCreateConversationRequest::ClientCreateConversationRequest(
    boost::json::object& obj)
    : ClientRequest(OperationCode::CLIENT_CREATE_CONVERSATION) {
    send_user_id = boost::json::value_to<Id>(obj.at("send_user_id"));
    recv_user_id = boost::json::value_to<Id>(obj.at("recv_user_id"));
}

ClientCreateConversationResponse::ClientCreateConversationResponse(
    StatusCode status_code, Id conversation_id, Id recv_user_id,
    String& recv_user_username)
    : ClientResponse(OperationCode::CLIENT_CREATE_CONVERSATION, status_code),
      conversation_id(conversation_id),
      recv_user_id(recv_user_id),
      recv_user_username(recv_user_username) {}

Data ClientCreateConversationResponse::serialize() const {
    boost::json::object obj = {
        {"operation_code", static_cast<int>(operation_code)},
        {"status_code", static_cast<int>(status_code)},
        {"conversation_id", conversation_id},
        {"recv_user_id", recv_user_id},
        {"recv_user_username", recv_user_username},
    };
    std::string res = boost::json::serialize(obj);
    Data data = initialize_serialization(res.size());
    data.insert(data.end(), res.begin(), res.end());
    return data;
}

ClientGetConversationsRequest::ClientGetConversationsRequest(
    boost::json::object& obj)
    : ClientRequest(OperationCode::CLIENT_GET_CONVERSATIONS) {
    user_id = boost::json::value_to<Id>(obj.at("user_id"));
}

ClientGetConversationsResponse::ClientGetConversationsResponse(
    StatusCode status_code,
    std::vector<std::tuple<Id, Id, String, Size>> conversations)
    : ClientResponse(OperationCode::CLIENT_GET_CONVERSATIONS, status_code),
      conversations(conversations) {}

Data ClientGetConversationsResponse::serialize() const {
    boost::json::object obj = {
        {"operation_code", static_cast<int>(operation_code)},
        {"status_code", static_cast<int>(status_code)},
    };
    boost::json::array convs_array;
    for (const auto& [id, send_user_id, recv_user_id, message_count] :
         conversations) {
        boost::json::object conv_obj = {
            {"id", id},
            {"send_user_id", send_user_id},
            {"recv_user_id", recv_user_id},
            {"message_count", message_count},
        };
        convs_array.emplace_back(conv_obj);
    }
    obj["conversations"] = convs_array;
    std::string res = boost::json::serialize(obj);
    Data data = initialize_serialization(res.size());
    data.insert(data.end(), res.begin(), res.end());
    return data;
}

ClientDeleteConversationRequest::ClientDeleteConversationRequest(
    boost::json::object& obj)
    : ClientRequest(OperationCode::CLIENT_DELETE_CONVERSATION) {
    conversation_id = boost::json::value_to<Id>(obj.at("conversation_id"));
}

ClientDeleteConversationResponse::ClientDeleteConversationResponse(
    StatusCode status_code)
    : ClientResponse(OperationCode::CLIENT_DELETE_CONVERSATION, status_code) {}

ClientSendMessageRequest::ClientSendMessageRequest(boost::json::object& obj)
    : ClientRequest(OperationCode::CLIENT_SEND_MESSAGE) {
    conversation_id = boost::json::value_to<Id>(obj.at("conversation_id"));
    send_user_id = boost::json::value_to<Id>(obj.at("sender_id"));
    content = boost::json::value_to<String>(obj.at("content"));
}

ClientSendMessageResponse::ClientSendMessageResponse(StatusCode status_code,
                                                     Id message_id)
    : ClientResponse(OperationCode::CLIENT_SEND_MESSAGE, status_code),
      message_id(message_id) {}

Data ClientSendMessageResponse::serialize() const {
    boost::json::object obj = {
        {"operation_code", static_cast<int>(operation_code)},
        {"status_code", static_cast<int>(status_code)},
        {"message_id", message_id},
    };
    std::string res = boost::json::serialize(obj);
    Data data = initialize_serialization(res.size());
    data.insert(data.end(), res.begin(), res.end());
    return data;
}

ClientGetMessagesRequest::ClientGetMessagesRequest(boost::json::object& obj)
    : ClientRequest(OperationCode::CLIENT_GET_MESSAGES) {
    conversation_id = boost::json::value_to<Id>(obj.at("conversation_id"));
}

ClientGetMessagesResponse::ClientGetMessagesResponse(
    StatusCode status_code,
    std::vector<std::tuple<Id, Id, Bool, String>> messages)
    : ClientResponse(OperationCode::CLIENT_GET_MESSAGES, status_code),
      messages(messages) {}

Data ClientGetMessagesResponse::serialize() const {
    boost::json::object obj = {
        {"operation_code", static_cast<int>(operation_code)},
        {"status_code", static_cast<int>(status_code)},
    };
    boost::json::array messages_array;
    for (const auto& [id, user_id, is_read, content] : messages) {
        boost::json::object msg_obj = {
            {"id", id},
            {"user_id", user_id},
            {"is_read", is_read},
            {"content", content},
        };
        messages_array.emplace_back(msg_obj);
    }
    obj["messages"] = messages_array;
    std::string res = boost::json::serialize(obj);
    Data data = initialize_serialization(res.size());
    data.insert(data.end(), res.begin(), res.end());
    return data;
}

ClientDeleteMessageRequest::ClientDeleteMessageRequest(boost::json::object& obj)
    : ClientRequest(OperationCode::CLIENT_DELETE_MESSAGE) {
    conversation_id = boost::json::value_to<Id>(obj.at("conversation_id"));
    message_id = boost::json::value_to<Id>(obj.at("message_id"));
}

ClientDeleteMessageResponse::ClientDeleteMessageResponse(StatusCode status_code)
    : ClientResponse(OperationCode::CLIENT_DELETE_MESSAGE, status_code) {}
