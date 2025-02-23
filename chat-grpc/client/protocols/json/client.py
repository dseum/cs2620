import struct
import json
from ..utils import OperationCode, StatusCode, SizeWrapper


class ClientRequest:
    operation_code: OperationCode

    def serialize(self) -> bytearray:
        res = json.dumps(
            {
                "operation_code": self.operation_code,
            },
            ensure_ascii=False,
        ).encode("utf-8")
        body_size = SizeWrapper(len(res))
        header = struct.pack("!" + body_size.format, body_size.value[0])
        return header + res

    def __str__(self) -> str:
        return ", ".join([f"{key}={value}" for key, value in self.__dict__.items()])

    def __repr__(self) -> str:
        return self.__str__()


class ClientResponse:
    operation_code: OperationCode
    status_code: StatusCode

    def __init__(self, obj: dict):
        self.status_code = obj["status_code"]

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
        res = json.dumps(
            {
                "operation_code": self.operation_code,
                "username": self.username,
                "password": self.password,
            },
            ensure_ascii=False,
        ).encode("utf-8")
        body_size = SizeWrapper(len(res))
        header = struct.pack("!" + body_size.format, body_size.value[0])
        return header + res


class SignupUserResponse(ClientResponse):
    operation_code = OperationCode.CLIENT_SIGNUP_USER
    user_id: int

    def __init__(self, obj: dict):
        self.status_code = obj["status_code"]
        if self.status_code == StatusCode.FAILURE:
            return
        self.user_id = obj["user_id"]


class SigninUserRequest(ClientRequest):
    operation_code = OperationCode.CLIENT_SIGNIN_USER

    def __init__(self, username: str, password: str):
        self.username = username
        self.password = password

    def serialize(self) -> bytearray:
        res = json.dumps(
            {
                "operation_code": self.operation_code,
                "username": self.username,
                "password": self.password,
            },
            ensure_ascii=False,
        ).encode("utf-8")
        print(res)
        body_size = SizeWrapper(len(res))
        header = struct.pack("!" + body_size.format, body_size.value[0])
        print(header, body_size.format, body_size.value[0])
        return header + res


class SigninUserResponse(ClientResponse):
    operation_code = OperationCode.CLIENT_SIGNIN_USER
    user_id: int

    def __init__(self, obj: dict):
        self.status_code = obj["status_code"]
        if self.status_code == StatusCode.FAILURE:
            return
        self.user_id = obj["user_id"]


class SignoutUserRequest(ClientRequest):
    operation_code = OperationCode.CLIENT_SIGNOUT_USER

    def __init__(self, user_id: int):
        self.user_id = user_id

    def serialize(self) -> bytearray:
        res = json.dumps(
            {
                "operation_code": self.operation_code,
                "user_id": self.user_id,
            },
            ensure_ascii=False,
        ).encode("utf-8")
        body_size = SizeWrapper(len(res))
        header = struct.pack("!" + body_size.format, body_size.value[0])
        return header + res


class SignoutUserResponse(ClientResponse):
    operation_code = OperationCode.CLIENT_SIGNOUT_USER


class DeleteUserRequest(ClientRequest):
    operation_code = OperationCode.CLIENT_DELETE_USER

    def __init__(self, user_id: int, password: str):
        self.user_id = user_id
        self.password = password

    def serialize(self) -> bytearray:
        res = json.dumps(
            {
                "operation_code": self.operation_code,
                "user_id": self.user_id,
                "password": self.password,
            },
            ensure_ascii=False,
        ).encode("utf-8")
        body_size = SizeWrapper(len(res))
        header = struct.pack("!" + body_size.format, body_size.value[0])
        return header + res


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
        res = json.dumps(
            {
                "operation_code": self.operation_code,
                "user_id": self.user_id,
                "query": self.query,
                "limit": self.limit,
                "offset": self.offset,
            },
            ensure_ascii=False,
        ).encode("utf-8")
        body_size = SizeWrapper(len(res))
        header = struct.pack("!" + body_size.format, body_size.value[0])
        return header + res


class GetOtherUsersResponse(ClientResponse):
    operation_code = OperationCode.CLIENT_GET_OTHER_USERS
    users: list[tuple[int, str]]

    def __init__(self, obj: dict):
        self.status_code = obj["status_code"]
        if self.status_code == StatusCode.FAILURE:
            return
        self.users = obj["users"]
        print(self.users)


class CreateConversationRequest(ClientRequest):
    operation_code = OperationCode.CLIENT_CREATE_CONVERSATION

    def __init__(self, send_user_id: int, recv_user_id: int):
        self.send_user_id = send_user_id
        self.recv_user_id = recv_user_id

    def serialize(self) -> bytearray:
        res = json.dumps(
            {
                "operation_code": self.operation_code,
                "send_user_id": self.send_user_id,
                "recv_user_id": self.recv_user_id,
            },
            ensure_ascii=False,
        ).encode("utf-8")
        body_size = SizeWrapper(len(res))
        header = struct.pack("!" + body_size.format, body_size.value[0])
        return header + res


class CreateConversationResponse(ClientResponse):
    operation_code = OperationCode.CLIENT_CREATE_CONVERSATION
    conversation_id: int
    recv_user_id: int
    recv_user_username: str

    def __init__(self, obj: dict):
        self.status_code = obj["status_code"]
        if self.status_code == StatusCode.FAILURE:
            return
        self.conversation_id = obj["conversation_id"]
        self.recv_user_id = obj["recv_user_id"]
        self.recv_user_username = obj["recv_user_username"]


class GetConversationsRequest(ClientRequest):
    operation_code = OperationCode.CLIENT_GET_CONVERSATIONS

    def __init__(self, user_id: int):
        self.user_id = user_id

    def serialize(self) -> bytearray:
        res = json.dumps(
            {
                "operation_code": self.operation_code,
                "user_id": self.user_id,
            },
            ensure_ascii=False,
        ).encode("utf-8")
        body_size = SizeWrapper(len(res))
        header = struct.pack("!" + body_size.format, body_size.value[0])
        return header + res


class GetConversationsResponse(ClientResponse):
    operation_code = OperationCode.CLIENT_GET_CONVERSATIONS
    conversations: list[tuple[int, int, str, int]]

    def __init__(self, obj: dict):
        self.status_code = obj["status_code"]
        if self.status_code == StatusCode.FAILURE:
            return
        self.conversations = obj["conversations"]


class DeleteConversationRequest(ClientRequest):
    operation_code = OperationCode.CLIENT_DELETE_CONVERSATION

    def __init__(self, conversation_id: int):
        self.conversation_id = conversation_id

    def serialize(self) -> bytearray:
        res = json.dumps(
            {
                "operation_code": self.operation_code,
                "conversation_id": self.conversation_id,
            },
            ensure_ascii=False,
        ).encode("utf-8")
        body_size = SizeWrapper(len(res))
        header = struct.pack("!" + body_size.format, body_size.value[0])
        return header + res


class DeleteConversationResponse(ClientResponse):
    operation_code = OperationCode.CLIENT_DELETE_CONVERSATION


class SendMessageRequest(ClientRequest):
    operation_code = OperationCode.CLIENT_SEND_MESSAGE

    def __init__(self, conversation_id: int, sender_id: int, content: str):
        self.conversation_id = conversation_id
        self.sender_id = sender_id
        self.content = content

    def serialize(self) -> bytearray:
        res = json.dumps(
            {
                "operation_code": self.operation_code,
                "conversation_id": self.conversation_id,
                "sender_id": self.sender_id,
                "content": self.content,
            },
            ensure_ascii=False,
        ).encode("utf-8")
        body_size = SizeWrapper(len(res))
        header = struct.pack("!" + body_size.format, body_size.value[0])
        return header + res


class SendMessageResponse(ClientResponse):
    operation_code = OperationCode.CLIENT_SEND_MESSAGE
    message_id: int

    def __init__(self, obj: dict):
        self.status_code = obj["status_code"]
        if self.status_code == StatusCode.FAILURE:
            return
        self.message_id = obj["message_id"]


class GetMessagesRequest(ClientRequest):
    operation_code = OperationCode.CLIENT_GET_MESSAGES

    def __init__(self, conversation_id: int):
        self.conversation_id = conversation_id

    def serialize(self) -> bytearray:
        res = json.dumps(
            {
                "operation_code": self.operation_code,
                "conversation_id": self.conversation_id,
            },
            ensure_ascii=False,
        ).encode("utf-8")
        body_size = SizeWrapper(len(res))
        header = struct.pack("!" + body_size.format, body_size.value[0])
        return header + res


class GetMessagesResponse(ClientResponse):
    operation_code = OperationCode.CLIENT_GET_MESSAGES
    messages: list[tuple[int, int, bool, str]]

    def __init__(self, obj: dict):
        self.status_code = obj["status_code"]
        if self.status_code == StatusCode.FAILURE:
            return
        self.messages = obj["messages"]


class DeleteMessageRequest(ClientRequest):
    operation_code = OperationCode.CLIENT_DELETE_MESSAGE

    def __init__(self, conversation_id: int, message_id: int):
        self.conversation_id = conversation_id
        self.message_id = message_id

    def serialize(self) -> bytearray:
        res = json.dumps(
            {
                "operation_code": self.operation_code,
                "conversation_id": self.conversation_id,
                "message_id": self.message_id,
            },
            ensure_ascii=False,
        ).encode("utf-8")
        body_size = SizeWrapper(len(res))
        header = struct.pack("!" + body_size.format, body_size.value[0])
        return header + res


class DeleteMessageResponse(ClientResponse):
    operation_code = OperationCode.CLIENT_DELETE_MESSAGE

