from typing import Union
from ..utils import OperationCodeWrapper, OperationCode
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
    operation_code, offset = OperationCodeWrapper.deserialize_from(data, 0)
    match operation_code:
        case OperationCode.CLIENT_SIGNUP_USER:
            return SignupUserResponse(data, offset)
        case OperationCode.CLIENT_SIGNIN_USER:
            return SigninUserResponse(data, offset)
        case OperationCode.CLIENT_SIGNOUT_USER:
            return SignoutUserResponse(data, offset)
        case OperationCode.CLIENT_DELETE_USER:
            return DeleteUserResponse(data, offset)
        case OperationCode.CLIENT_GET_OTHER_USERS:
            return GetOtherUsersResponse(data, offset)
        case OperationCode.CLIENT_CREATE_CONVERSATION:
            return CreateConversationResponse(data, offset)
        case OperationCode.CLIENT_GET_CONVERSATIONS:
            return GetConversationsResponse(data, offset)
        case OperationCode.CLIENT_DELETE_CONVERSATION:
            return DeleteConversationResponse(data, offset)
        case OperationCode.CLIENT_SEND_MESSAGE:
            return SendMessageResponse(data, offset)
        case OperationCode.CLIENT_GET_MESSAGES:
            return GetMessagesResponse(data, offset)
        case OperationCode.CLIENT_DELETE_MESSAGE:
            return DeleteMessageResponse(data, offset)
        case OperationCode.SERVER_SEND_MESSAGE:
            return SendMessageRequest(data, offset)
        case OperationCode.SERVER_UPDATE_UNREAD_COUNT:
            return UpdateUnreadCountRequest(data, offset)
