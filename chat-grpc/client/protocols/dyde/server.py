from ..utils import (
    OperationCode,
    SizeWrapper,
    IdWrapper,
    StringWrapper,
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

    def __init__(self, data: bytearray, offset: int):
        self.conversation_id, offset = IdWrapper.deserialize_from(data, offset)
        self.message_id, offset = IdWrapper.deserialize_from(data, offset)
        self.send_user_id, offset = IdWrapper.deserialize_from(data, offset)
        self.content, offset = StringWrapper.deserialize_from(data, offset)


class UpdateUnreadCountRequest(ServerRequest):
    operation_code = OperationCode.SERVER_UPDATE_UNREAD_COUNT

    def __init__(self, data: bytearray, offset: int):
        self.conversation_id, offset = IdWrapper.deserialize_from(data, offset)
        self.unread_count, offset = SizeWrapper.deserialize_from(data, offset)
