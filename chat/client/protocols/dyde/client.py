from ..utils import (
    serialize,
    OperationCode,
    StatusCode,
    BoolWrapper,
    SizeWrapper,
    IdWrapper,
    StringWrapper,
    OperationCodeWrapper,
    StatusCodeWrapper,
)


class ClientRequest:
    operation_code: OperationCode

    def serialize(self) -> bytearray:
        return serialize(OperationCodeWrapper(self.operation_code))

    def __str__(self) -> str:
        return ", ".join([f"{key}={value}" for key, value in self.__dict__.items()])

    def __repr__(self) -> str:
        return self.__str__()


class ClientResponse:
    operation_code: OperationCode
    status_code: StatusCode

    def __init__(self, data: bytearray, offset: int):
        self.status_code, _ = StatusCodeWrapper.deserialize_from(data, offset)

    def __str__(self) -> str:
        return ", ".join([f"{key}={value}" for key, value in self.__dict__.items()])

    def __repr__(self) -> str:
        return self.__str__()


class SignupUserRequest(ClientRequest):
    operation_code = OperationCode.CLIENT_SIGNUP_USER

    def __init__(self, username: str, password: str):
        self.username = username
        self.password = password

    def serialize(self) -> bytearray:
        return serialize(
            OperationCodeWrapper(self.operation_code),
            StringWrapper(self.username),
            StringWrapper(self.password),
        )


class SignupUserResponse(ClientResponse):
    operation_code = OperationCode.CLIENT_SIGNUP_USER
    user_id: int

    def __init__(self, data: bytearray, offset: int):
        self.status_code, offset = StatusCodeWrapper.deserialize_from(data, offset)
        if self.status_code == StatusCode.FAILURE:
            return
        self.user_id, _ = IdWrapper.deserialize_from(data, offset)


class SigninUserRequest(ClientRequest):
    operation_code = OperationCode.CLIENT_SIGNIN_USER

    def __init__(self, username: str, password: str):
        self.username = username
        self.password = password

    def serialize(self) -> bytearray:
        return serialize(
            OperationCodeWrapper(self.operation_code),
            StringWrapper(self.username),
            StringWrapper(self.password),
        )


class SigninUserResponse(ClientResponse):
    operation_code = OperationCode.CLIENT_SIGNIN_USER
    user_id: int

    def __init__(self, data: bytearray, offset: int):
        self.status_code, offset = StatusCodeWrapper.deserialize_from(data, offset)
        if self.status_code == StatusCode.FAILURE:
            return
        self.user_id, _ = IdWrapper.deserialize_from(data, offset)


class SignoutUserRequest(ClientRequest):
    operation_code = OperationCode.CLIENT_SIGNOUT_USER

    def __init__(self, user_id: int):
        self.user_id = user_id

    def serialize(self) -> bytearray:
        return serialize(
            OperationCodeWrapper(self.operation_code),
            IdWrapper(self.user_id),
        )


class SignoutUserResponse(ClientResponse):
    operation_code = OperationCode.CLIENT_SIGNOUT_USER


class DeleteUserRequest(ClientRequest):
    operation_code = OperationCode.CLIENT_DELETE_USER

    def __init__(self, user_id: int, password: str):
        self.user_id = user_id
        self.password = password

    def serialize(self) -> bytearray:
        return serialize(
            OperationCodeWrapper(self.operation_code),
            IdWrapper(self.user_id),
            StringWrapper(self.password),
        )


class DeleteUserResponse(ClientResponse):
    operation_code = OperationCode.CLIENT_DELETE_USER


class GetOtherUsersRequest(ClientRequest):
    operation_code = OperationCode.CLIENT_GET_OTHER_USERS

    def __init__(
        self,
        user_id: int,
        query: str,
        limit: int,
        offset: int,
    ):
        self.user_id = user_id
        self.query = query
        self.limit = limit
        self.offset = offset

    def serialize(self) -> bytearray:
        return serialize(
            OperationCodeWrapper(self.operation_code),
            IdWrapper(self.user_id),
            StringWrapper(self.query),
            SizeWrapper(self.limit),
            SizeWrapper(self.offset),
        )


class GetOtherUsersResponse(ClientResponse):
    operation_code = OperationCode.CLIENT_GET_OTHER_USERS
    users: list[tuple[int, str]]

    def __init__(self, data: bytearray, offset: int):
        self.status_code, offset = StatusCodeWrapper.deserialize_from(data, offset)
        if self.status_code == StatusCode.FAILURE:
            return
        self.users = []
        users_size, offset = SizeWrapper.deserialize_from(data, offset)
        for _ in range(users_size):
            user_id, offset = IdWrapper.deserialize_from(data, offset)
            username, offset = StringWrapper.deserialize_from(data, offset)
            self.users.append((user_id, username))


class CreateConversationRequest(ClientRequest):
    operation_code = OperationCode.CLIENT_CREATE_CONVERSATION

    def __init__(self, send_user_id: int, recv_user_id: int):
        self.send_user_id = send_user_id
        self.recv_user_id = recv_user_id

    def serialize(self) -> bytearray:
        return serialize(
            OperationCodeWrapper(self.operation_code),
            IdWrapper(self.send_user_id),
            IdWrapper(self.recv_user_id),
        )


class CreateConversationResponse(ClientResponse):
    operation_code = OperationCode.CLIENT_CREATE_CONVERSATION
    conversation_id: int
    recv_user_id: int
    recv_user_username: str

    def __init__(self, data: bytearray, offset: int):
        self.status_code, offset = StatusCodeWrapper.deserialize_from(data, offset)
        if self.status_code == StatusCode.FAILURE:
            return
        self.conversation_id, offset = IdWrapper.deserialize_from(data, offset)
        self.recv_user_id, offset = IdWrapper.deserialize_from(data, offset)
        self.recv_user_username, _ = StringWrapper.deserialize_from(data, offset)


class GetConversationsRequest(ClientRequest):
    operation_code = OperationCode.CLIENT_GET_CONVERSATIONS

    def __init__(self, user_id: int):
        self.user_id = user_id

    def serialize(self) -> bytearray:
        return serialize(
            OperationCodeWrapper(self.operation_code),
            IdWrapper(self.user_id),
        )


class GetConversationsResponse(ClientResponse):
    operation_code = OperationCode.CLIENT_GET_CONVERSATIONS
    conversations: list[tuple[int, int, str, int]]

    def __init__(self, data: bytearray, offset: int):
        self.status_code, offset = StatusCodeWrapper.deserialize_from(data, offset)
        if self.status_code == StatusCode.FAILURE:
            return
        self.conversations = []
        conversations_size, offset = SizeWrapper.deserialize_from(data, offset)
        for _ in range(conversations_size):
            conversation_id, offset = IdWrapper.deserialize_from(data, offset)
            user_id, offset = IdWrapper.deserialize_from(data, offset)
            username, offset = StringWrapper.deserialize_from(data, offset)
            unread_count, offset = SizeWrapper.deserialize_from(data, offset)
            self.conversations.append(
                (conversation_id, user_id, username, unread_count)
            )


class DeleteConversationRequest(ClientRequest):
    operation_code = OperationCode.CLIENT_DELETE_CONVERSATION

    def __init__(self, conversation_id: int):
        self.conversation_id = conversation_id

    def serialize(self) -> bytearray:
        return serialize(
            OperationCodeWrapper(self.operation_code),
            IdWrapper(self.conversation_id),
        )


class DeleteConversationResponse(ClientResponse):
    operation_code = OperationCode.CLIENT_DELETE_CONVERSATION


class SendMessageRequest(ClientRequest):
    operation_code = OperationCode.CLIENT_SEND_MESSAGE

    def __init__(self, conversation_id: int, sender_id: int, content: str):
        self.conversation_id = conversation_id
        self.sender_id = sender_id
        self.content = content

    def serialize(self) -> bytearray:
        return serialize(
            OperationCodeWrapper(self.operation_code),
            IdWrapper(self.conversation_id),
            IdWrapper(self.sender_id),
            StringWrapper(self.content),
        )


class SendMessageResponse(ClientResponse):
    operation_code = OperationCode.CLIENT_SEND_MESSAGE
    message_id: int

    def __init__(self, data: bytearray, offset: int):
        self.status_code, _ = StatusCodeWrapper.deserialize_from(data, offset)
        if self.status_code == StatusCode.FAILURE:
            return
        self.message_id, _ = IdWrapper.deserialize_from(data, offset)


class GetMessagesRequest(ClientRequest):
    operation_code = OperationCode.CLIENT_GET_MESSAGES

    def __init__(self, conversation_id: int):
        self.conversation_id = conversation_id

    def serialize(self) -> bytearray:
        return serialize(
            OperationCodeWrapper(self.operation_code),
            IdWrapper(self.conversation_id),
        )


class GetMessagesResponse(ClientResponse):
    operation_code = OperationCode.CLIENT_GET_MESSAGES
    messages: list[tuple[int, int, bool, str]]

    def __init__(self, data: bytearray, offset: int):
        self.status_code, offset = StatusCodeWrapper.deserialize_from(data, offset)
        if self.status_code == StatusCode.FAILURE:
            return
        self.messages = []
        messages_size, offset = SizeWrapper.deserialize_from(data, offset)
        for _ in range(messages_size):
            message_id, offset = IdWrapper.deserialize_from(data, offset)
            send_user_id, offset = IdWrapper.deserialize_from(data, offset)
            is_read, offset = BoolWrapper.deserialize_from(data, offset)
            content, offset = StringWrapper.deserialize_from(data, offset)
            self.messages.append((message_id, send_user_id, is_read, content))


class DeleteMessageRequest(ClientRequest):
    operation_code = OperationCode.CLIENT_DELETE_MESSAGE

    def __init__(self, conversation_id: int, message_id: int):
        self.conversation_id = conversation_id
        self.message_id = message_id

    def serialize(self) -> bytearray:
        return serialize(
            OperationCodeWrapper(self.operation_code),
            IdWrapper(self.conversation_id),
            IdWrapper(self.message_id),
        )


class DeleteMessageResponse(ClientResponse):
    operation_code = OperationCode.CLIENT_DELETE_MESSAGE
