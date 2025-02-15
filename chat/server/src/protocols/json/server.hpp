#pragma once

#include "utils.hpp"

class ServerRequest {
   public:
    OperationCode operation_code;

    ServerRequest(OperationCode operation_code);

    virtual Data serialize() const;
};

class ServerSendMessageRequest : public ServerRequest {
   public:
    Id conversation_id;
    Id message_id;
    Id send_user_id;
    String content;

    ServerSendMessageRequest(Id conversation_id, Id messagge_id,
                             Id send_user_id, const String& content);
    Data serialize() const;
};

class ServerUpdateUnreadCountRequest : public ServerRequest {
   public:
    Id conversation_id;
    Size unread_count;

    ServerUpdateUnreadCountRequest(Id conversation_id, Size unread_count);
    Data serialize() const;
};
