#include "server.hpp"

#include <boost/json.hpp>

#include "../utils.hpp"

ServerRequest::ServerRequest(OperationCode operation_code)
    : operation_code(operation_code) {}

Data ServerRequest::serialize() const {
    boost::json::object obj = {
        {"operation_code", static_cast<int>(operation_code)}};
    std::string res = boost::json::serialize(obj);
    Data data = initialize_serialization(res.size());
    data.insert(data.end(), res.begin(), res.end());
    return data;
}

ServerSendMessageRequest::ServerSendMessageRequest(Id conversation_id,
                                                   Id message_id,
                                                   Id send_user_id,
                                                   const String& content)
    : ServerRequest(OperationCode::SERVER_SEND_MESSAGE),
      conversation_id(conversation_id),
      message_id(message_id),
      send_user_id(send_user_id),
      content(content) {}

Data ServerSendMessageRequest::serialize() const {
    boost::json::object obj = {
        {"operation_code", static_cast<int>(operation_code)},
        {"conversation_id", conversation_id},
        {"message_id", message_id},
        {"send_user_id", send_user_id},
        {"content", content},
    };
    std::string res = boost::json::serialize(obj);
    Data data = initialize_serialization(res.size());
    data.insert(data.end(), res.begin(), res.end());
    return data;
}

ServerUpdateUnreadCountRequest::ServerUpdateUnreadCountRequest(
    Id conversation_id, Size unread_count)
    : ServerRequest(OperationCode::SERVER_UPDATE_UNREAD_COUNT),
      conversation_id(conversation_id),
      unread_count(unread_count) {}

Data ServerUpdateUnreadCountRequest::serialize() const {
    boost::json::object obj = {
        {"operation_code", static_cast<int>(operation_code)},
        {"conversation_id", conversation_id},
        {"unread_count", unread_count},
    };
    std::string res = boost::json::serialize(obj);
    Data data = initialize_serialization(res.size());
    data.insert(data.end(), res.begin(), res.end());
    return data;
}
