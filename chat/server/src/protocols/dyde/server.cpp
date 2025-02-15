#include "server.hpp"

#include "../utils.hpp"

ServerRequest::ServerRequest(OperationCode operation_code)
    : operation_code(operation_code) {}

Data ServerRequest::serialize() const {
    size_t size = OperationCodeWrapper::size();
    Data data = initialize_serialization(size);
    OperationCodeWrapper::serialize_to(data, operation_code);
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
    size_t size = OperationCodeWrapper::size() + IdWrapper::size() +
                  IdWrapper::size() + IdWrapper::size() +
                  StringWrapper::size(content);
    Data data = initialize_serialization(size);
    OperationCodeWrapper::serialize_to(data, operation_code);
    IdWrapper::serialize_to(data, conversation_id);
    IdWrapper::serialize_to(data, message_id);
    IdWrapper::serialize_to(data, send_user_id);
    StringWrapper::serialize_to(data, content);
    return data;
}

ServerUpdateUnreadCountRequest::ServerUpdateUnreadCountRequest(
    Id conversation_id, Size unread_count)
    : ServerRequest(OperationCode::SERVER_UPDATE_UNREAD_COUNT),
      conversation_id(conversation_id),
      unread_count(unread_count) {}

Data ServerUpdateUnreadCountRequest::serialize() const {
    size_t size =
        OperationCodeWrapper::size() + IdWrapper::size() + SizeWrapper::size();
    Data data = initialize_serialization(size);
    OperationCodeWrapper::serialize_to(data, operation_code);
    IdWrapper::serialize_to(data, conversation_id);
    SizeWrapper::serialize_to(data, unread_count);
    return data;
}
