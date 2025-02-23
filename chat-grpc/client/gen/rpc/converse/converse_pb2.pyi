from google.protobuf.internal import containers as _containers
from google.protobuf import descriptor as _descriptor
from google.protobuf import message as _message
from typing import ClassVar as _ClassVar, Iterable as _Iterable, Mapping as _Mapping, Optional as _Optional, Union as _Union

DESCRIPTOR: _descriptor.FileDescriptor

class Message(_message.Message):
    __slots__ = ("message_id", "sender_id", "is_read", "content")
    MESSAGE_ID_FIELD_NUMBER: _ClassVar[int]
    SENDER_ID_FIELD_NUMBER: _ClassVar[int]
    IS_READ_FIELD_NUMBER: _ClassVar[int]
    CONTENT_FIELD_NUMBER: _ClassVar[int]
    message_id: int
    sender_id: int
    is_read: bool
    content: str
    def __init__(self, message_id: _Optional[int] = ..., sender_id: _Optional[int] = ..., is_read: bool = ..., content: _Optional[str] = ...) -> None: ...

class Conversation(_message.Message):
    __slots__ = ("conversation_id", "recv_user_id", "recv_username", "unread_count")
    CONVERSATION_ID_FIELD_NUMBER: _ClassVar[int]
    RECV_USER_ID_FIELD_NUMBER: _ClassVar[int]
    RECV_USERNAME_FIELD_NUMBER: _ClassVar[int]
    UNREAD_COUNT_FIELD_NUMBER: _ClassVar[int]
    conversation_id: int
    recv_user_id: int
    recv_username: str
    unread_count: int
    def __init__(self, conversation_id: _Optional[int] = ..., recv_user_id: _Optional[int] = ..., recv_username: _Optional[str] = ..., unread_count: _Optional[int] = ...) -> None: ...

class User(_message.Message):
    __slots__ = ("user_id", "username")
    USER_ID_FIELD_NUMBER: _ClassVar[int]
    USERNAME_FIELD_NUMBER: _ClassVar[int]
    user_id: int
    username: str
    def __init__(self, user_id: _Optional[int] = ..., username: _Optional[str] = ...) -> None: ...

class SignupUserRequest(_message.Message):
    __slots__ = ("username", "password")
    USERNAME_FIELD_NUMBER: _ClassVar[int]
    PASSWORD_FIELD_NUMBER: _ClassVar[int]
    username: str
    password: str
    def __init__(self, username: _Optional[str] = ..., password: _Optional[str] = ...) -> None: ...

class SignupUserResponse(_message.Message):
    __slots__ = ("user_id",)
    USER_ID_FIELD_NUMBER: _ClassVar[int]
    user_id: int
    def __init__(self, user_id: _Optional[int] = ...) -> None: ...

class SigninUserRequest(_message.Message):
    __slots__ = ("username", "password")
    USERNAME_FIELD_NUMBER: _ClassVar[int]
    PASSWORD_FIELD_NUMBER: _ClassVar[int]
    username: str
    password: str
    def __init__(self, username: _Optional[str] = ..., password: _Optional[str] = ...) -> None: ...

class SigninUserResponse(_message.Message):
    __slots__ = ("user_id",)
    USER_ID_FIELD_NUMBER: _ClassVar[int]
    user_id: int
    def __init__(self, user_id: _Optional[int] = ...) -> None: ...

class SendMessageRequest(_message.Message):
    __slots__ = ("conversation_id", "sender_id", "content")
    CONVERSATION_ID_FIELD_NUMBER: _ClassVar[int]
    SENDER_ID_FIELD_NUMBER: _ClassVar[int]
    CONTENT_FIELD_NUMBER: _ClassVar[int]
    conversation_id: int
    sender_id: int
    content: str
    def __init__(self, conversation_id: _Optional[int] = ..., sender_id: _Optional[int] = ..., content: _Optional[str] = ...) -> None: ...

class SendMessageResponse(_message.Message):
    __slots__ = ("message_id",)
    MESSAGE_ID_FIELD_NUMBER: _ClassVar[int]
    message_id: int
    def __init__(self, message_id: _Optional[int] = ...) -> None: ...

class SignoutUserRequest(_message.Message):
    __slots__ = ("user_id",)
    USER_ID_FIELD_NUMBER: _ClassVar[int]
    user_id: int
    def __init__(self, user_id: _Optional[int] = ...) -> None: ...

class SignoutUserResponse(_message.Message):
    __slots__ = ()
    def __init__(self) -> None: ...

class DeleteUserRequest(_message.Message):
    __slots__ = ("user_id", "password")
    USER_ID_FIELD_NUMBER: _ClassVar[int]
    PASSWORD_FIELD_NUMBER: _ClassVar[int]
    user_id: int
    password: str
    def __init__(self, user_id: _Optional[int] = ..., password: _Optional[str] = ...) -> None: ...

class DeleteUserResponse(_message.Message):
    __slots__ = ()
    def __init__(self) -> None: ...

class GetOtherUsersRequest(_message.Message):
    __slots__ = ("user_id", "query", "limit", "offset")
    USER_ID_FIELD_NUMBER: _ClassVar[int]
    QUERY_FIELD_NUMBER: _ClassVar[int]
    LIMIT_FIELD_NUMBER: _ClassVar[int]
    OFFSET_FIELD_NUMBER: _ClassVar[int]
    user_id: int
    query: str
    limit: int
    offset: int
    def __init__(self, user_id: _Optional[int] = ..., query: _Optional[str] = ..., limit: _Optional[int] = ..., offset: _Optional[int] = ...) -> None: ...

class GetOtherUsersResponse(_message.Message):
    __slots__ = ("users",)
    USERS_FIELD_NUMBER: _ClassVar[int]
    users: _containers.RepeatedCompositeFieldContainer[User]
    def __init__(self, users: _Optional[_Iterable[_Union[User, _Mapping]]] = ...) -> None: ...

class CreateConversationRequest(_message.Message):
    __slots__ = ("user_id", "other_user_id")
    USER_ID_FIELD_NUMBER: _ClassVar[int]
    OTHER_USER_ID_FIELD_NUMBER: _ClassVar[int]
    user_id: int
    other_user_id: int
    def __init__(self, user_id: _Optional[int] = ..., other_user_id: _Optional[int] = ...) -> None: ...

class CreateConversationResponse(_message.Message):
    __slots__ = ("conversation_id",)
    CONVERSATION_ID_FIELD_NUMBER: _ClassVar[int]
    conversation_id: int
    def __init__(self, conversation_id: _Optional[int] = ...) -> None: ...

class GetConversationRequest(_message.Message):
    __slots__ = ("user_id",)
    USER_ID_FIELD_NUMBER: _ClassVar[int]
    user_id: int
    def __init__(self, user_id: _Optional[int] = ...) -> None: ...

class GetConversationResponse(_message.Message):
    __slots__ = ("conversations",)
    CONVERSATIONS_FIELD_NUMBER: _ClassVar[int]
    conversations: _containers.RepeatedCompositeFieldContainer[Conversation]
    def __init__(self, conversations: _Optional[_Iterable[_Union[Conversation, _Mapping]]] = ...) -> None: ...

class DeleteConversationRequest(_message.Message):
    __slots__ = ("user_id", "conversation_id")
    USER_ID_FIELD_NUMBER: _ClassVar[int]
    CONVERSATION_ID_FIELD_NUMBER: _ClassVar[int]
    user_id: int
    conversation_id: int
    def __init__(self, user_id: _Optional[int] = ..., conversation_id: _Optional[int] = ...) -> None: ...

class DeleteConversationResponse(_message.Message):
    __slots__ = ()
    def __init__(self) -> None: ...

class GetMessagesRequest(_message.Message):
    __slots__ = ("conversation_id", "last_message_id")
    CONVERSATION_ID_FIELD_NUMBER: _ClassVar[int]
    LAST_MESSAGE_ID_FIELD_NUMBER: _ClassVar[int]
    conversation_id: int
    last_message_id: int
    def __init__(self, conversation_id: _Optional[int] = ..., last_message_id: _Optional[int] = ...) -> None: ...

class GetMessagesResponse(_message.Message):
    __slots__ = ("messages",)
    MESSAGES_FIELD_NUMBER: _ClassVar[int]
    messages: _containers.RepeatedCompositeFieldContainer[Message]
    def __init__(self, messages: _Optional[_Iterable[_Union[Message, _Mapping]]] = ...) -> None: ...

class DeleteMessageRequest(_message.Message):
    __slots__ = ("conversation_id", "message_id")
    CONVERSATION_ID_FIELD_NUMBER: _ClassVar[int]
    MESSAGE_ID_FIELD_NUMBER: _ClassVar[int]
    conversation_id: int
    message_id: int
    def __init__(self, conversation_id: _Optional[int] = ..., message_id: _Optional[int] = ...) -> None: ...

class DeleteMessageResponse(_message.Message):
    __slots__ = ()
    def __init__(self) -> None: ...

class ReceiveMessageRequest(_message.Message):
    __slots__ = ("conversation_id",)
    CONVERSATION_ID_FIELD_NUMBER: _ClassVar[int]
    conversation_id: int
    def __init__(self, conversation_id: _Optional[int] = ...) -> None: ...
