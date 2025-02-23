#include "json_client.hpp"

#include <boost/json.hpp>
#include <cstdint>

// Define our json alias for Boost.JSON.
using json = boost::json::value;

// -----------------------------------------------------------------------------
// Base class constructor definitions
// -----------------------------------------------------------------------------
JsonClientRequest::JsonClientRequest(JsonClientOperationCode op_code)
    : operation_code(op_code) {}

JsonClientResponse::JsonClientResponse(JsonClientOperationCode op_code,
                                       JsonStatusCode status_code)
    : operation_code(op_code), status_code(status_code) {}

// -----------------------------------------------------------------------------
// Helper: Serialize a JSON payload with a 4-byte size header (big-endian).
// -----------------------------------------------------------------------------
std::string serialize_with_size(const json &j) {
    // Serialize the JSON payload.
    std::string payload = boost::json::serialize(j);
    // Get its size.
    uint32_t size = static_cast<uint32_t>(payload.size());
    // Encode size as 4 bytes (big-endian).
    char header[4];
    header[0] = (size >> 24) & 0xFF;
    header[1] = (size >> 16) & 0xFF;
    header[2] = (size >> 8) & 0xFF;
    header[3] = size & 0xFF;
    // Build final message.
    std::string result;
    result.append(header, 4);
    result.append(payload);
    return result;
}

// -----------------------------------------------------------------------------
// Base Classes Destructors and to_json()
// -----------------------------------------------------------------------------
JsonClientRequest::~JsonClientRequest() {}

JsonClientResponse::~JsonClientResponse() {}

boost::json::value JsonClientResponse::to_json() const {
    boost::json::object obj;
    obj["operation_code"] = static_cast<int>(operation_code);
    obj["status_code"] = static_cast<int>(status_code);
    return obj;
}

// -----------------------------------------------------------------------------
// Signup User
// -----------------------------------------------------------------------------
JsonSignupUserRequest::JsonSignupUserRequest(const String &user,
                                             const String &pass)
    : JsonClientRequest(JsonClientOperationCode::SIGNUP_USER),
      username(user),
      password(pass) {}

JsonSignupUserRequest::JsonSignupUserRequest(const json &j)
    : JsonClientRequest(JsonClientOperationCode::SIGNUP_USER) {
    const auto &obj = j.as_object();
    username = boost::json::value_to<String>(obj.at("username"));
    password = boost::json::value_to<String>(obj.at("password"));
}

boost::json::value JsonSignupUserRequest::to_json() const {
    boost::json::object j;
    j["operation_code"] = static_cast<int>(operation_code);
    j["username"] = username;
    j["password"] = password;
    return j;
}

JsonSignupUserResponse::JsonSignupUserResponse(JsonStatusCode status,
                                               ChatId userId)
    : JsonClientResponse(JsonClientOperationCode::SIGNUP_USER, status),
      user_id(userId) {}

JsonSignupUserResponse::JsonSignupUserResponse(const json &j)
    : JsonClientResponse(JsonClientOperationCode::SIGNUP_USER,
                         static_cast<JsonStatusCode>(boost::json::value_to<int>(
                             j.as_object().at("status_code")))) {
    user_id = boost::json::value_to<ChatId>(j.as_object().at("user_id"));
}

boost::json::value JsonSignupUserResponse::to_json() const {
    boost::json::object j = JsonClientResponse::to_json().as_object();
    j["user_id"] = user_id;
    return j;
}

// -----------------------------------------------------------------------------
// Signin User
// -----------------------------------------------------------------------------
JsonSigninUserRequest::JsonSigninUserRequest(const String &user,
                                             const String &pass)
    : JsonClientRequest(JsonClientOperationCode::SIGNIN_USER),
      username(user),
      password(pass) {}

JsonSigninUserRequest::JsonSigninUserRequest(const json &j)
    : JsonClientRequest(JsonClientOperationCode::SIGNIN_USER) {
    const auto &obj = j.as_object();
    username = boost::json::value_to<String>(obj.at("username"));
    password = boost::json::value_to<String>(obj.at("password"));
}

boost::json::value JsonSigninUserRequest::to_json() const {
    boost::json::object j;
    j["operation_code"] = static_cast<int>(operation_code);
    j["username"] = username;
    j["password"] = password;
    return j;
}

JsonSigninUserResponse::JsonSigninUserResponse(JsonStatusCode status,
                                               ChatId userId)
    : JsonClientResponse(JsonClientOperationCode::SIGNIN_USER, status),
      user_id(userId) {}

JsonSigninUserResponse::JsonSigninUserResponse(const json &j)
    : JsonClientResponse(JsonClientOperationCode::SIGNIN_USER,
                         static_cast<JsonStatusCode>(boost::json::value_to<int>(
                             j.as_object().at("status_code")))) {
    user_id = boost::json::value_to<ChatId>(j.as_object().at("user_id"));
}

boost::json::value JsonSigninUserResponse::to_json() const {
    boost::json::object j = JsonClientResponse::to_json().as_object();
    j["user_id"] = user_id;
    return j;
}

// -----------------------------------------------------------------------------
// Signout User
// -----------------------------------------------------------------------------
JsonSignoutUserRequest::JsonSignoutUserRequest(ChatId uid)
    : JsonClientRequest(JsonClientOperationCode::SIGNOUT_USER), user_id(uid) {}

JsonSignoutUserRequest::JsonSignoutUserRequest(const json &j)
    : JsonClientRequest(JsonClientOperationCode::SIGNOUT_USER) {
    user_id = boost::json::value_to<ChatId>(j.as_object().at("user_id"));
}

boost::json::value JsonSignoutUserRequest::to_json() const {
    boost::json::object j;
    j["operation_code"] = static_cast<int>(operation_code);
    j["user_id"] = user_id;
    return j;
}

JsonSignoutUserResponse::JsonSignoutUserResponse(JsonStatusCode status)
    : JsonClientResponse(JsonClientOperationCode::SIGNOUT_USER, status) {}

JsonSignoutUserResponse::JsonSignoutUserResponse(const json &j)
    : JsonClientResponse(JsonClientOperationCode::SIGNOUT_USER,
                         static_cast<JsonStatusCode>(boost::json::value_to<int>(
                             j.as_object().at("status_code")))) {}

boost::json::value JsonSignoutUserResponse::to_json() const {
    return JsonClientResponse::to_json();
}

// -----------------------------------------------------------------------------
// Delete User
// -----------------------------------------------------------------------------
JsonDeleteUserRequest::JsonDeleteUserRequest(ChatId uid, const String &pass)
    : JsonClientRequest(JsonClientOperationCode::DELETE_USER),
      user_id(uid),
      password(pass) {}

JsonDeleteUserRequest::JsonDeleteUserRequest(const json &j)
    : JsonClientRequest(JsonClientOperationCode::DELETE_USER) {
    const auto &obj = j.as_object();
    user_id = boost::json::value_to<ChatId>(obj.at("user_id"));
    password = boost::json::value_to<String>(obj.at("password"));
}

boost::json::value JsonDeleteUserRequest::to_json() const {
    boost::json::object j;
    j["operation_code"] = static_cast<int>(operation_code);
    j["user_id"] = user_id;
    j["password"] = password;
    return j;
}

JsonDeleteUserResponse::JsonDeleteUserResponse(JsonStatusCode status)
    : JsonClientResponse(JsonClientOperationCode::DELETE_USER, status) {}

JsonDeleteUserResponse::JsonDeleteUserResponse(const json &j)
    : JsonClientResponse(JsonClientOperationCode::DELETE_USER,
                         static_cast<JsonStatusCode>(boost::json::value_to<int>(
                             j.as_object().at("status_code")))) {}

boost::json::value JsonDeleteUserResponse::to_json() const {
    return JsonClientResponse::to_json();
}

// -----------------------------------------------------------------------------
// Get Other Users
// -----------------------------------------------------------------------------
JsonGetOtherUsersRequest::JsonGetOtherUsersRequest(ChatId userId,
                                                   const String &query,
                                                   Size limit, Size offset)
    : JsonClientRequest(JsonClientOperationCode::GET_OTHER_USERS),
      user_id(userId),
      query(query),
      limit(limit),
      offset(offset) {}

JsonGetOtherUsersRequest::JsonGetOtherUsersRequest(const boost::json::value &j)
    : JsonClientRequest(JsonClientOperationCode::GET_OTHER_USERS) {
    auto obj = j.as_object();
    user_id = obj.at("user_id").as_int64();
    query = std::string(obj.at("query").as_string());
    // Cast JSON integer to Size (std::size_t)
    limit = static_cast<Size>(obj.at("limit").as_int64());
    offset = static_cast<Size>(obj.at("offset").as_int64());
}

boost::json::value JsonGetOtherUsersRequest::to_json() const {
    boost::json::object obj;
    obj["operation_code"] = static_cast<int>(operation_code);
    obj["user_id"] = user_id;
    obj["query"] = query;
    obj["limit"] = static_cast<int64_t>(limit);
    obj["offset"] = static_cast<int64_t>(offset);
    return obj;
}

// -----------------------------------------------------------------------------
// Implementation for JsonGetOtherUsersResponse
// -----------------------------------------------------------------------------
JsonGetOtherUsersResponse::JsonGetOtherUsersResponse(
    JsonStatusCode status, const std::vector<boost::json::value> &users)
    : JsonClientResponse(JsonClientOperationCode::GET_OTHER_USERS, status),
      users(users) {}

JsonGetOtherUsersResponse::JsonGetOtherUsersResponse(
    const boost::json::value &j)
    : JsonClientResponse(
          JsonClientOperationCode::GET_OTHER_USERS,
          JsonStatusCode::ERROR)  // default status; will be overwritten
{
    auto obj = j.as_object();
    status_code = static_cast<JsonStatusCode>(obj.at("status_code").as_int64());
    auto arr = obj.at("users").as_array();
    for (const auto &u : arr) {
        users.push_back(u);
    }
}

boost::json::value JsonGetOtherUsersResponse::to_json() const {
    boost::json::object obj;
    obj["operation_code"] = static_cast<int>(operation_code);
    obj["status_code"] = static_cast<int>(status_code);
    boost::json::array arr;
    for (const auto &user : users) {
        arr.push_back(user);
    }
    obj["users"] = arr;
    return obj;
}

// -----------------------------------------------------------------------------
// Create Conversation
// -----------------------------------------------------------------------------
JsonCreateConversationRequest::JsonCreateConversationRequest(ChatId sender,
                                                             ChatId receiver)
    : JsonClientRequest(JsonClientOperationCode::CREATE_CONVERSATION),
      send_user_id(sender),
      recv_user_id(receiver) {}

JsonCreateConversationRequest::JsonCreateConversationRequest(const json &j)
    : JsonClientRequest(JsonClientOperationCode::CREATE_CONVERSATION) {
    const auto &obj = j.as_object();
    send_user_id = boost::json::value_to<ChatId>(obj.at("send_user_id"));
    recv_user_id = boost::json::value_to<ChatId>(obj.at("recv_user_id"));
}

boost::json::value JsonCreateConversationRequest::to_json() const {
    boost::json::object j;
    j["operation_code"] = static_cast<int>(operation_code);
    j["send_user_id"] = send_user_id;
    j["recv_user_id"] = recv_user_id;
    return j;
}

JsonCreateConversationResponse::JsonCreateConversationResponse(
    JsonStatusCode status, ChatId convId)
    : JsonClientResponse(JsonClientOperationCode::CREATE_CONVERSATION, status),
      conversation_id(convId) {}

JsonCreateConversationResponse::JsonCreateConversationResponse(const json &j)
    : JsonClientResponse(JsonClientOperationCode::CREATE_CONVERSATION,
                         static_cast<JsonStatusCode>(boost::json::value_to<int>(
                             j.as_object().at("status_code")))) {
    conversation_id =
        boost::json::value_to<ChatId>(j.as_object().at("conversation_id"));
}

boost::json::value JsonCreateConversationResponse::to_json() const {
    boost::json::object j = JsonClientResponse::to_json().as_object();
    j["conversation_id"] = conversation_id;
    return j;
}

// -----------------------------------------------------------------------------
// Get Conversations
// -----------------------------------------------------------------------------
JsonGetConversationsRequest::JsonGetConversationsRequest(ChatId uid)
    : JsonClientRequest(JsonClientOperationCode::GET_CONVERSATIONS),
      user_id(uid) {}

JsonGetConversationsRequest::JsonGetConversationsRequest(const json &j)
    : JsonClientRequest(JsonClientOperationCode::GET_CONVERSATIONS) {
    user_id = boost::json::value_to<ChatId>(j.as_object().at("user_id"));
}

boost::json::value JsonGetConversationsRequest::to_json() const {
    boost::json::object j;
    j["operation_code"] = static_cast<int>(operation_code);
    j["user_id"] = user_id;
    return j;
}

JsonGetConversationsResponse::JsonGetConversationsResponse(
    JsonStatusCode status, const std::vector<json> &convs)
    : JsonClientResponse(JsonClientOperationCode::GET_CONVERSATIONS, status),
      conversations(convs) {}

JsonGetConversationsResponse::JsonGetConversationsResponse(const json &j)
    : JsonClientResponse(JsonClientOperationCode::GET_CONVERSATIONS,
                         static_cast<JsonStatusCode>(boost::json::value_to<int>(
                             j.as_object().at("status_code")))) {
    const auto &obj = j.as_object();
    const auto &arr = obj.at("conversations").as_array();
    conversations.clear();
    for (const auto &item : arr) conversations.push_back(item);
}

boost::json::value JsonGetConversationsResponse::to_json() const {
    boost::json::object j = JsonClientResponse::to_json().as_object();
    boost::json::array arr;
    for (const auto &conv : conversations) arr.push_back(conv);
    j["conversations"] = arr;
    return j;
}

// -----------------------------------------------------------------------------
// Delete Conversation
// -----------------------------------------------------------------------------
JsonDeleteConversationRequest::JsonDeleteConversationRequest(ChatId convId)
    : JsonClientRequest(JsonClientOperationCode::DELETE_CONVERSATION),
      conversation_id(convId) {}

JsonDeleteConversationRequest::JsonDeleteConversationRequest(const json &j)
    : JsonClientRequest(JsonClientOperationCode::DELETE_CONVERSATION) {
    conversation_id =
        boost::json::value_to<ChatId>(j.as_object().at("conversation_id"));
}

boost::json::value JsonDeleteConversationRequest::to_json() const {
    boost::json::object j;
    j["operation_code"] = static_cast<int>(operation_code);
    j["conversation_id"] = conversation_id;
    return j;
}

JsonDeleteConversationResponse::JsonDeleteConversationResponse(
    JsonStatusCode status)
    : JsonClientResponse(JsonClientOperationCode::DELETE_CONVERSATION, status) {
}

JsonDeleteConversationResponse::JsonDeleteConversationResponse(const json &j)
    : JsonClientResponse(JsonClientOperationCode::DELETE_CONVERSATION,
                         static_cast<JsonStatusCode>(boost::json::value_to<int>(
                             j.as_object().at("status_code")))) {}

boost::json::value JsonDeleteConversationResponse::to_json() const {
    return JsonClientResponse::to_json();
}

// -----------------------------------------------------------------------------
// Send Message
// -----------------------------------------------------------------------------
JsonSendMessageRequest::JsonSendMessageRequest(ChatId convId, ChatId senderId,
                                               const String &msg)
    : JsonClientRequest(JsonClientOperationCode::SEND_MESSAGE),
      conversation_id(convId),
      send_user_id(senderId),
      content(msg) {}

JsonSendMessageRequest::JsonSendMessageRequest(const json &j)
    : JsonClientRequest(JsonClientOperationCode::SEND_MESSAGE) {
    const auto &obj = j.as_object();
    conversation_id = boost::json::value_to<ChatId>(obj.at("conversation_id"));
    send_user_id = boost::json::value_to<ChatId>(obj.at("send_user_id"));
    content = boost::json::value_to<String>(obj.at("content"));
}

boost::json::value JsonSendMessageRequest::to_json() const {
    boost::json::object j;
    j["operation_code"] = static_cast<int>(operation_code);
    j["conversation_id"] = conversation_id;
    j["send_user_id"] = send_user_id;
    j["content"] = content;
    return j;
}

JsonSendMessageResponse::JsonSendMessageResponse(JsonStatusCode status,
                                                 ChatId msgId)
    : JsonClientResponse(JsonClientOperationCode::SEND_MESSAGE, status),
      message_id(msgId) {}

JsonSendMessageResponse::JsonSendMessageResponse(const json &j)
    : JsonClientResponse(JsonClientOperationCode::SEND_MESSAGE,
                         static_cast<JsonStatusCode>(boost::json::value_to<int>(
                             j.as_object().at("status_code")))) {
    message_id = boost::json::value_to<ChatId>(j.as_object().at("message_id"));
}

boost::json::value JsonSendMessageResponse::to_json() const {
    boost::json::object j = JsonClientResponse::to_json().as_object();
    j["message_id"] = message_id;
    return j;
}

// -----------------------------------------------------------------------------
// Get Messages
// -----------------------------------------------------------------------------
JsonGetMessagesRequest::JsonGetMessagesRequest(ChatId convId, ChatId lastMsgId)
    : JsonClientRequest(JsonClientOperationCode::GET_MESSAGES),
      conversation_id(convId),
      last_message_id(lastMsgId) {}

JsonGetMessagesRequest::JsonGetMessagesRequest(const json &j)
    : JsonClientRequest(JsonClientOperationCode::GET_MESSAGES) {
    const auto &obj = j.as_object();
    conversation_id = boost::json::value_to<ChatId>(obj.at("conversation_id"));
    last_message_id = boost::json::value_to<ChatId>(obj.at("last_message_id"));
}

boost::json::value JsonGetMessagesRequest::to_json() const {
    boost::json::object j;
    j["operation_code"] = static_cast<int>(operation_code);
    j["conversation_id"] = conversation_id;
    j["last_message_id"] = last_message_id;
    return j;
}

JsonGetMessagesResponse::JsonGetMessagesResponse(JsonStatusCode status,
                                                 const std::vector<json> &msgs)
    : JsonClientResponse(JsonClientOperationCode::GET_MESSAGES, status),
      messages(msgs) {}

JsonGetMessagesResponse::JsonGetMessagesResponse(const json &j)
    : JsonClientResponse(JsonClientOperationCode::GET_MESSAGES,
                         static_cast<JsonStatusCode>(boost::json::value_to<int>(
                             j.as_object().at("status_code")))) {
    const auto &obj = j.as_object();
    const auto &arr = obj.at("messages").as_array();
    messages.clear();
    for (const auto &item : arr) messages.push_back(item);
}

boost::json::value JsonGetMessagesResponse::to_json() const {
    boost::json::object j = JsonClientResponse::to_json().as_object();
    boost::json::array arr;
    for (const auto &msg : messages) arr.push_back(msg);
    j["messages"] = arr;
    return j;
}

// -----------------------------------------------------------------------------
// Delete Message
// -----------------------------------------------------------------------------
JsonDeleteMessageRequest::JsonDeleteMessageRequest(ChatId convId, ChatId msgId)
    : JsonClientRequest(JsonClientOperationCode::DELETE_MESSAGE),
      conversation_id(convId),
      message_id(msgId) {}

JsonDeleteMessageRequest::JsonDeleteMessageRequest(const json &j)
    : JsonClientRequest(JsonClientOperationCode::DELETE_MESSAGE) {
    const auto &obj = j.as_object();
    conversation_id = boost::json::value_to<ChatId>(obj.at("conversation_id"));
    message_id = boost::json::value_to<ChatId>(obj.at("message_id"));
}

boost::json::value JsonDeleteMessageRequest::to_json() const {
    boost::json::object j;
    j["operation_code"] = static_cast<int>(operation_code);
    j["conversation_id"] = conversation_id;
    j["message_id"] = message_id;
    return j;
}

JsonDeleteMessageResponse::JsonDeleteMessageResponse(JsonStatusCode status)
    : JsonClientResponse(JsonClientOperationCode::DELETE_MESSAGE, status) {}

JsonDeleteMessageResponse::JsonDeleteMessageResponse(const json &j)
    : JsonClientResponse(JsonClientOperationCode::DELETE_MESSAGE,
                         static_cast<JsonStatusCode>(boost::json::value_to<int>(
                             j.as_object().at("status_code")))) {}

boost::json::value JsonDeleteMessageResponse::to_json() const {
    return JsonClientResponse::to_json();
}
