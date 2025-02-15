from PySide6.QtCore import (
    Qt,
    QAbstractListModel,
    QModelIndex,
    Slot,
    QEnum,
    QByteArray,
    QObject,
    Signal,
    Property,
)
from PySide6.QtNetwork import QTcpSocket, QAbstractSocket
from enum import IntEnum
from protocols.utils import SizeWrapper, OperationCode, StatusCode


from protocols.dyde.deserialize import deserialize
from protocols.dyde.client import (
    SignupUserRequest,
    SigninUserRequest,
    SignoutUserRequest,
    DeleteUserRequest,
    GetOtherUsersRequest,
    CreateConversationRequest,
    GetConversationsRequest,
    DeleteConversationRequest,
    SendMessageRequest,
    GetMessagesRequest,
    DeleteMessageRequest,
)


QML_IMPORT_NAME = "Backend"
QML_IMPORT_MAJOR_VERSION = 1


class User(QObject):
    state = Signal()

    def __init__(self, id: int, username: str):
        super().__init__()
        self.m_id = id
        self.m_username = username

    @Property(int, notify=state)
    def id(self):
        return self.m_id

    @id.setter
    def id(self, value):
        if self.m_id == value:
            return
        self.m_id = value
        self.state.emit()

    @Property(str, notify=state)
    def username(self):
        return self.m_username

    @username.setter
    def username(self, value):
        if self.m_username == value:
            return
        self.m_username = value
        self.state.emit()


class Message(QObject):
    state = Signal()

    def __init__(self, id: int, send_user_id: int, is_read: bool, content: str):
        super().__init__()
        self.m_id = id
        self.m_send_user_id = send_user_id
        self.m_is_read = is_read
        self.m_content = content

    @Property(int, notify=state)
    def id(self):
        return self.m_id

    @id.setter
    def id(self, value):
        if self.m_id == value:
            return
        self.m_id = value
        self.state.emit()

    @Property(int, notify=state)
    def sendUserId(self):
        return self.m_send_user_id

    @sendUserId.setter
    def sendUserId(self, value):
        if self.m_send_user_id == value:
            return
        self.m_send_user_id = value
        self.state.emit()

    @Property(bool, notify=state)
    def isRead(self):
        return self.m_is_read

    @isRead.setter
    def isRead(self, value):
        if self.m_is_read == value:
            return
        self.m_is_read = value
        self.state.emit()

    @Property(str, notify=state)
    def content(self):
        return self.m_content


class Conversation(QObject):
    state = Signal()

    def __init__(
        self, id: int, recv_user_id: int, recv_user_username: str, unread_count: int = 0
    ):
        super().__init__()
        self.m_id = id
        self.m_recv_user_id = recv_user_id
        self.m_recv_user_username = recv_user_username
        self.m_unread_count = unread_count

    @Property(int, notify=state)
    def id(self):
        return self.m_id

    @id.setter
    def id(self, value):
        if self.m_id == value:
            return
        self.m_id = value
        self.state.emit()

    @Property(int, notify=state)
    def recvUserId(self):
        return self.m_recv_user_id

    @recvUserId.setter
    def recvUserId(self, value):
        if self.m_recv_user_id == value:
            return
        self.m_recv_user_id = value
        self.state.emit()

    @Property(str, notify=state)
    def recvUserUsername(self):
        return self.m_recv_user_username

    @recvUserUsername.setter
    def recvUserUsername(self, value):
        if self.m_recv_user_username == value:
            return
        self.m_recv_user_username = value
        self.state.emit()

    @Property(int, notify=state)
    def unreadCount(self):
        return self.m_unread_count

    @unreadCount.setter
    def unreadCount(self, value):
        if self.m_unread_count == value:
            return
        self.m_unread_count = value
        self.state.emit()


class ConversationsModel(QAbstractListModel):
    @QEnum
    class ConversationRole(IntEnum):
        IdRole = Qt.UserRole
        RecvUserIdRole = Qt.UserRole + 1
        RecvUserUsernameRole = Qt.UserRole + 2
        UnreadCountRole = Qt.UserRole + 3

    def __init__(self, parent=None):
        super().__init__(parent)
        self.m_conversations = []

    def rowCount(self, parent=QModelIndex()):
        return len(self.m_conversations)

    def data(self, index: QModelIndex, role: int):
        row = index.row()
        if row < self.rowCount():
            conversation = self.m_conversations[row]
            match role:
                case self.ConversationRole.IdRole:
                    return conversation.id
                case self.ConversationRole.RecvUserIdRole:
                    return conversation.recvUserId
                case self.ConversationRole.RecvUserUsernameRole:
                    return conversation.recvUserUsername
                case self.ConversationRole.UnreadCountRole:
                    return conversation.unreadCount
        return None

    def roleNames(self):
        roles = super().roleNames()
        roles[self.ConversationRole.IdRole] = QByteArray(b"id")
        roles[self.ConversationRole.RecvUserIdRole] = QByteArray(b"recvUserId")
        roles[self.ConversationRole.RecvUserUsernameRole] = QByteArray(
            b"recvUserUsername"
        )
        roles[self.ConversationRole.UnreadCountRole] = QByteArray(b"unreadCount")
        return roles

    @Slot(int, result="QVariantMap")
    def get(self, index):
        conversation = self.m_conversations[index]
        return {
            "id": conversation.id,
            "recvUserId": conversation.recvUserId,
            "recvUserUsername": conversation.recvUserUsername,
            "unreadCount": conversation.unreadCount,
        }

    @Slot(int)
    def removeConversationAt(self, index):
        if index < 0 or index >= len(self.m_conversations):
            return
        self.beginRemoveRows(QModelIndex(), index, index)
        self.m_conversations.pop(index)
        self.endRemoveRows()

    @Slot(Conversation)
    def prepend(self, conversation):
        self.beginInsertRows(QModelIndex(), 0, 0)
        self.m_conversations.insert(0, conversation)
        self.endInsertRows()

    @Slot(Conversation)
    def append(self, conversation):
        self.beginInsertRows(QModelIndex(), self.rowCount(), self.rowCount())
        self.m_conversations.append(conversation)
        self.endInsertRows()

    @Slot(list)
    def reset(self, conversations):
        self.beginResetModel()
        self.m_conversations = conversations
        self.endResetModel()


class MessagesModel(QAbstractListModel):
    @QEnum
    class MessageRoles(IntEnum):
        IdRole = Qt.UserRole
        SendUserIdRole = Qt.UserRole + 1
        IsReadRole = Qt.UserRole + 2
        ContentRole = Qt.UserRole + 3

    def __init__(self, parent=None):
        super().__init__(parent)
        self.m_messages = []

    def rowCount(self, parent=QModelIndex()):
        return len(self.m_messages)

    def data(self, index: QModelIndex, role: int):
        row = index.row()
        if row < self.rowCount():
            message = self.m_messages[row]

            match role:
                case self.MessageRoles.IdRole:
                    return message.id
                case self.MessageRoles.SendUserIdRole:
                    return message.sendUserId
                case self.MessageRoles.IsReadRole:
                    return message.isRead
                case self.MessageRoles.ContentRole:
                    return message.content
        return None

    @Slot(int)
    def removeMessageAt(self, index):
        if index < 0 or index >= len(self.m_messages):
            return
        self.beginRemoveRows(QModelIndex(), index, index)
        self.m_messages.pop(index)
        self.endRemoveRows()

    def roleNames(self):
        roles = super().roleNames()
        roles[self.MessageRoles.IdRole] = QByteArray(b"id")
        roles[self.MessageRoles.SendUserIdRole] = QByteArray(b"sendUserId")
        roles[self.MessageRoles.IsReadRole] = QByteArray(b"isRead")
        roles[self.MessageRoles.ContentRole] = QByteArray(b"content")
        return roles

    @Slot(int, result="QVariantMap")
    def get(self, index):
        message = self.m_messages[index]
        return {
            "id": message.id,
            "sendUserId": message.sendUserId,
            "isRead": message.isRead,
            "content": message.content,
        }

    @Slot(Message)
    def append(self, value):
        self.beginInsertRows(QModelIndex(), self.rowCount(), self.rowCount())
        self.m_messages.append(value)
        self.endInsertRows()

    @Slot(list)
    def reset(self, messages):
        self.beginResetModel()
        self.m_messages = messages
        self.endResetModel()


class OtherUsersModel(QAbstractListModel):
    @QEnum
    class OtherUserRoles(IntEnum):
        IdRole = Qt.UserRole
        UsernameRole = Qt.UserRole + 1

    def __init__(self, parent=None):
        super().__init__(parent)
        self.m_other_users = []

    def rowCount(self, parent=QModelIndex()):
        return len(self.m_other_users)

    def data(self, index: QModelIndex, role: int):
        row = index.row()
        if row < self.rowCount():
            other_user = self.m_other_users[row]
            match role:
                case self.OtherUserRoles.IdRole:
                    return other_user.id
                case self.OtherUserRoles.UsernameRole:
                    return other_user.username
        return None

    def roleNames(self):
        roles = super().roleNames()
        roles[self.OtherUserRoles.IdRole] = QByteArray(b"id")
        roles[self.OtherUserRoles.UsernameRole] = QByteArray(b"username")
        return roles

    @Slot(int, result="QVariantMap")
    def get(self, index):
        other_user = self.m_other_users[index]
        return {
            "id": other_user.id,
            "username": other_user.username,
        }

    @Slot(int, str)
    def append(self, id, username):
        other_user = self.OtherUser(id, username)
        self.beginInsertRows(QModelIndex(), 0, 0)
        self.m_other_users.insert(0, other_user)
        self.endInsertRows()

    @Slot(list)
    def reset(self, other_users):
        self.beginResetModel()
        self.m_other_users = other_users
        self.endResetModel()


class Backend(QObject):
    userChanged = Signal()
    conversationChanged = Signal()

    clientSigninUserResponded = Signal(bool)
    clientSignupUserResponded = Signal(bool)
    clientSignoutUserResponded = Signal(bool)
    clientDeleteUserResponded = Signal(bool)
    clientGetOtherUsersResponded = Signal(bool, list)
    clientCreateConversationResponded = Signal(bool, Conversation)
    clientGetConversationsResponded = Signal(bool, list)
    clientDeleteConversationResponded = Signal(bool)
    clientSendMessageResponded = Signal(bool)
    clientGetMessagesResponded = Signal(bool, list)
    clientDeleteMessageResponded = Signal(bool)

    serverSendMessageRequested = Signal(Message)
    serverUpdateUnreadCountRequested = Signal(int)

    def __init__(self, host: str, port: int, parent=None):
        super().__init__(parent)
        self.m_user = None
        self.m_conversation = None

        self.socket = QTcpSocket(self)
        self.socket.readyRead.connect(self.recv)
        self.socket.connected.connect(self.onConnected)
        self.socket.errorOccurred.connect(self.onErrorOccurred)
        self.socket.connectToHost(host, port)

        self.buffer = bytearray()
        self.expected_size = None
        self.request = None
        self.response = None

    @Property(User, notify=userChanged)
    def user(self):
        return self.m_user

    @user.setter
    def user(self, value):
        if self.m_user is not None and value is not None and self.m_user.id == value.id:
            return
        self.m_user = value
        self.userChanged.emit()

    @Property(Conversation, notify=conversationChanged)
    def conversation(self):
        return self.m_conversation

    @conversation.setter
    def conversation(self, value):
        if (
            self.m_conversation is not None
            and value is not None
            and self.m_conversation.id == value.id
        ):
            return
        self.m_conversation = value
        self.conversationChanged.emit()

    @Slot(int, int, str)
    def setConversation(self, id, recv_user_id, recv_user_username):
        self.conversation = Conversation(id, recv_user_id, recv_user_username)

    def send(self, data: bytearray):
        print("send", len(data))
        self.socket.write(data)
        self.socket.flush()

    @Slot()
    def recv(self):
        header_size = SizeWrapper.size()
        while self.socket.bytesAvailable() > 0:
            data = self.socket.readAll()
            self.buffer.extend(data)
            while True:
                # If we haven't yet read the header, try to read it.
                if self.expected_size is None:
                    if len(self.buffer) >= header_size:
                        # Deserialize the size from the beginning of the buffer.
                        self.expected_size, offset = SizeWrapper.deserialize_from(
                            self.buffer, 0
                        )

                        # Remove the header bytes.
                        self.buffer = self.buffer[offset:]
                    else:
                        break  # Wait for more data.
                    # If we have the full payload, emit it.
                if (
                    self.expected_size is not None
                    and len(self.buffer) >= self.expected_size
                ):
                    message_data = self.buffer[: self.expected_size]
                    self.buffer = self.buffer[self.expected_size :]
                    self.expected_size = None  # Reset for the next message.
                    self.handle(message_data)
                else:
                    break

    def handle(self, data: bytearray):
        body = deserialize(data)
        print("recv", len(data), body)
        match body.operation_code:
            case OperationCode.CLIENT_SIGNUP_USER:
                if body.status_code == StatusCode.FAILURE:
                    self.clientSignupUserResponded.emit(False)
                    return
                self.clientSignupUserResponded.emit(True)
                self.user = User(body.user_id, self.request.username)
            case OperationCode.CLIENT_SIGNIN_USER:
                if body.status_code == StatusCode.FAILURE:
                    self.clientSigninUserResponded.emit(False)
                    return
                self.clientSigninUserResponded.emit(True)
                self.user = User(body.user_id, self.request.username)
            case OperationCode.CLIENT_SIGNOUT_USER:
                if body.status_code == StatusCode.FAILURE:
                    self.clientSignoutUserResponded.emit(False)
                    return
                self.clientSignoutUserResponded.emit(True)
                self.user = None
            case OperationCode.CLIENT_DELETE_USER:
                if body.status_code == StatusCode.FAILURE:
                    self.clientDeleteUserResponded.emit(False)
                    return
                self.clientDeleteUserResponded.emit(True)
                self.user = None
            case OperationCode.CLIENT_GET_OTHER_USERS:
                if body.status_code == StatusCode.FAILURE:
                    self.clientGetOtherUsersResponded.emit(False, [])
                    return
                users = [
                    User(user[0], user[1]) for user in body.users
                ]  # This line is going to be commented out for json
                self.clientGetOtherUsersResponded.emit(True, users)
            case OperationCode.CLIENT_CREATE_CONVERSATION:
                if body.status_code == StatusCode.FAILURE:
                    self.clientCreateConversationResponded.emit(False, None)
                    return
                self.conversation = Conversation(
                    body.conversation_id,
                    body.recv_user_id,
                    body.recv_user_username,
                )
                self.clientCreateConversationResponded.emit(
                    True,
                    Conversation(
                        body.conversation_id,
                        body.recv_user_id,
                        body.recv_user_username,
                    ),
                )
            case OperationCode.CLIENT_GET_CONVERSATIONS:
                if body.status_code == StatusCode.FAILURE:
                    self.clientGetConversationsResponded.emit(False)
                    return
                conversations = [
                    Conversation(
                        conversation[0],
                        conversation[1],
                        conversation[2],
                        conversation[3],
                    )
                    for conversation in body.conversations
                ]
                self.clientGetConversationsResponded.emit(True, conversations)
            case OperationCode.CLIENT_DELETE_CONVERSATION:
                if body.status_code == StatusCode.FAILURE:
                    self.clientDeleteConversationResponded.emit(False)
                    return
                self.clientDeleteConversationResponded.emit(True)
                self.conversation = None
            case OperationCode.CLIENT_SEND_MESSAGE:
                if body.status_code == StatusCode.FAILURE:
                    self.clientSendMessageResponded.emit(False)
                    return
                self.clientSendMessageResponded.emit(True)
            case OperationCode.CLIENT_GET_MESSAGES:
                if body.status_code == StatusCode.FAILURE:
                    self.clientGetMessagesResponded.emit(False)
                    return
                messages = [
                    Message(
                        message[0],
                        message[1],
                        message[2],
                        message[3],
                    )
                    for message in body.messages
                    if message[3].strip() != ""
                ]
                self.clientGetMessagesResponded.emit(True, messages)
            case OperationCode.CLIENT_DELETE_MESSAGE:
                if body.status_code == StatusCode.FAILURE:
                    self.clientDeleteMessageResponded.emit(False)
                else:
                    self.clientDeleteMessageResponded.emit(True)
            case OperationCode.SERVER_SEND_MESSAGE:
                if body.conversation_id == self.conversation.id:
                    if not body.content.strip():
                        return
                    self.serverSendMessageRequested.emit(
                        Message(
                            body.message_id,
                            body.send_user_id,
                            False,
                            body.content,
                        ),
                    )
            case OperationCode.SERVER_UPDATE_UNREAD_COUNT:
                print(1, body)

    @Slot()
    def onConnected(self):
        print("Connected")

    @Slot(QAbstractSocket.SocketError)
    def onErrorOccurred(self, error):
        print(f"Error: {self.socket.errorString()}")

    @Slot(str, str)
    def mSignIn(self, username: str, password: str):
        self.request = SigninUserRequest(username, password)
        self.send(self.request.serialize())

    @Slot(str, str)
    def mSignUp(self, username: str, password: str):
        self.request = SignupUserRequest(username, password)
        self.send(self.request.serialize())

    @Slot()
    def mSignOut(self):
        self.request = SignoutUserRequest(self.user.id)
        self.send(self.request.serialize())

    @Slot(str)
    def mDeleteUser(self, password: str):
        self.request = DeleteUserRequest(self.user.id, password)
        self.send(self.request.serialize())

    @Slot(str)
    def mGetOtherUsers(self, query: str):
        self.request = GetOtherUsersRequest(self.user.id, query, 10, 0)
        self.send(self.request.serialize())

    @Slot(int)
    def mCreateConversation(self, recv_user_id: int):
        self.request = CreateConversationRequest(self.user.id, recv_user_id)
        self.send(self.request.serialize())

    @Slot()
    def mGetConversations(self):
        self.request = GetConversationsRequest(self.user.id)
        self.send(self.request.serialize())

    @Slot(int)
    def mDeleteConversation(self, conversation_id: int):
        self.request = DeleteConversationRequest(conversation_id)
        self.send(self.request.serialize())

    @Slot(str)
    def mSendMessage(self, content: str):
        self.request = SendMessageRequest(self.conversation.id, self.user.id, content)
        self.send(self.request.serialize())

    @Slot()
    def mGetMessages(self):
        self.request = GetMessagesRequest(self.conversation.id)
        self.send(self.request.serialize())

    @Slot(int)
    def mDeleteMessage(self, message_id: int):
        conversation_id = self.conversation.id
        self.request = DeleteMessageRequest(conversation_id, message_id)
        self.send(self.request.serialize())

    @Slot()
    def mCheck(self):
        print(self.buffer, self.socket.bytesAvailable())
