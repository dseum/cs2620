import json
from typing import Union
from ..utils import OperationCode
from .client import (
    ClientResponse,
    SignupUserResponse,
    SigninUserResponse,
    SignoutUserResponse,
    DeleteUserResponse,
    GetOtherUsersResponse,
    CreateConversationResponse,
    GetConversationsResponse,
    DeleteConversationResponse,
    SendMessageResponse,
    GetMessagesResponse,
    DeleteMessageResponse,
)
from .server import ServerRequest, SendMessageRequest, UpdateUnreadCountRequest


def deserialize(data: bytearray) -> Union[ClientResponse, ServerRequest]:
    obj = json.loads(data)
    operation_code = obj["operation_code"]
    match operation_code:
        case OperationCode.CLIENT_SIGNUP_USER:
            return SignupUserResponse(obj)
        case OperationCode.CLIENT_SIGNIN_USER:
            return SigninUserResponse(obj)
        case OperationCode.CLIENT_SIGNOUT_USER:
            return SignoutUserResponse(obj)
        case OperationCode.CLIENT_DELETE_USER:
            return DeleteUserResponse(obj)
        case OperationCode.CLIENT_GET_OTHER_USERS:
            return GetOtherUsersResponse(obj)
        case OperationCode.CLIENT_CREATE_CONVERSATION:
            return CreateConversationResponse(obj)
        case OperationCode.CLIENT_GET_CONVERSATIONS:
            return GetConversationsResponse(obj)
        case OperationCode.CLIENT_DELETE_CONVERSATION:
            return DeleteConversationResponse(obj)
        case OperationCode.CLIENT_SEND_MESSAGE:
            return SendMessageResponse(obj)
        case OperationCode.CLIENT_GET_MESSAGES:
            return GetMessagesResponse(obj)
        case OperationCode.CLIENT_DELETE_MESSAGE:
            return DeleteMessageResponse(obj)
        case OperationCode.SERVER_SEND_MESSAGE:
            return SendMessageRequest(obj)
        case OperationCode.SERVER_UPDATE_UNREAD_COUNT:
            return UpdateUnreadCountRequest(obj)
