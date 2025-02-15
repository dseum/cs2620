import json
from ..utils import (
    OperationCode,
)


class ServerRequest:
    operation_code: OperationCode

    def __init__(self, data: bytearray, offset: int):
        pass

    def __str__(self) -> str:
        return ", ".join([f"{key}={value}" for key, value in self.__dict__.items()])

    def __repr__(self) -> str:
        return self.__str__()


class SendMessageRequest(ServerRequest):
    operation_code = OperationCode.SERVER_SEND_MESSAGE

    def __init__(self, obj: dict):
        self.conversation_id = obj["conversation_id"]
        self.message_id = obj["message_id"]
        self.send_user_id = obj["send_user_id"]
        self.content = obj["content"]


class UpdateUnreadCountRequest(ServerRequest):
    operation_code = OperationCode.SERVER_UPDATE_UNREAD_COUNT

    def __init__(self, obj: dict):
        self.conversation_id = obj["conversation_id"]
        self.unread_count = obj["unread_count"]
