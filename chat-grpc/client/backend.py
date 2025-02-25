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
from enum import IntEnum
from gen.rpc.converse.converse_pb2_grpc import ConverseServiceStub
import gen.rpc.converse.converse_pb2 as pb2
import grpc
import asyncio
from PySide6.QtCore import QThread


def ms(message):
    return len(message.SerializeToString())


QML_IMPORT_NAME = "Backend"
QML_IMPORT_MAJOR_VERSION = 1


class AsyncioThread(QThread):
    def __init__(self):
        super().__init__()
        self.loop = None

    def run(self):
        """Run an asyncio event loop in a separate thread."""
        self.loop = asyncio.new_event_loop()
        asyncio.set_event_loop(self.loop)
        self.loop.run_forever()

    def stop(self):
        """Stop the event loop safely."""
        if self.loop:
            self.loop.call_soon_threadsafe(self.loop.stop)
        self.quit()
        self.wait()


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
    toBottom = Signal()

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
        self.toBottom.emit()

    @Slot(list)
    def reset(self, messages):
        self.beginResetModel()
        self.m_messages = messages
        self.endResetModel()
        self.toBottom.emit()


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

        self.channel = grpc.insecure_channel("localhost:54100")
        self.stub = ConverseServiceStub(self.channel)
        self.stream = None
        self.stream_inner = None
        self.asyncio_thread = AsyncioThread()
        self.asyncio_thread.start()

    def __del__(self):
        if self.stream_inner is not None:
            self.stream_inner.cancel()
            self.stream_inner = None
        if self.stream is not None:
            self.stream.cancel()
            self.stream = None
        if self.asyncio_thread:
            self.asyncio_thread.stop()
        if self.channel is not None:
            self.channel.close()
            self.channel = None

    @Property(User, notify=userChanged)
    def user(self):
        return self.m_user

    async def receive_message(self, user_id):
        print("stream start for" + str(user_id))
        self.stream_inner = self.stub.ReceiveMessage(
            pb2.ReceiveMessageRequest(user_id=user_id)
        )
        try:
            for item in self.stream_inner:
                conversation_id = item.conversation_id
                if (
                    self.conversation is not None
                    and conversation_id == self.conversation.id
                ):
                    message = item.message
                    self.serverSendMessageRequested.emit(
                        Message(
                            message.message_id,
                            message.send_user_id,
                            message.is_read,
                            message.content,
                        )
                    )
                else:
                    print("unread count update")
        except grpc.RpcError as e:
            self.stream_inner = None
            print(e)
            print("stream end for" + str(user_id))

    def start_receive_message(self, user_id):
        if self.asyncio_thread.loop:
            self.stream = asyncio.run_coroutine_threadsafe(
                self.receive_message(user_id), self.asyncio_thread.loop
            )

    @user.setter
    def user(self, value):
        if self.m_user is not None and value is not None and self.m_user.id == value.id:
            return
        self.m_user = value
        if self.stream is not None:
            self.stream_inner.cancel()
            print("stream cancel")
            self.stream = None
        if value is not None:
            self.start_receive_message(value.id)
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

    @Slot(str, str)
    def mSignIn(self, username: str, password: str):
        try:
            req = pb2.SigninUserRequest(username=username, password=password)
            res = self.stub.SigninUser(req)
            print(f"SigninUser :req: {ms(req)} :res: {ms(res)}")
            self.user = User(res.user_id, username)
            self.clientSigninUserResponded.emit(True)
        except grpc.RpcError as e:
            print(e)
            self.clientSigninUserResponded.emit(False)

    @Slot(str, str)
    def mSignUp(self, username: str, password: str):
        try:
            req = pb2.SignupUserRequest(username=username, password=password)
            res = self.stub.SignupUser(res)
            print(f"SignupUser :req: {ms(req)} :res: {ms(res)}")
            self.user = User(res.user_id, username)
            self.clientSignupUserResponded.emit(True)
        except grpc.RpcError as e:
            print(e)
            self.clientSignupUserResponded.emit(False)

    @Slot()
    def mSignOut(self):
        try:
            req = pb2.SignoutUserRequest(user_id=self.user.id)
            res = self.stub.SignoutUser(req)
            print(f"SignoutUser :req: {ms(req)} :res: {ms(res)}")
            self.user = None
            self.clientSignoutUserResponded.emit(True)
        except grpc.RpcError as e:
            print(e)
            self.clientSignoutUserResponded.emit(False)

    @Slot(str)
    def mDeleteUser(self, password: str):
        try:
            req = pb2.DeleteUserRequest(user_id=self.user.id, password=password)
            res = self.stub.DeleteUser(req)
            print(f"DeleteUser :req: {ms(req)} :res: {ms(res)}")
            self.user = None
            self.clientDeleteUserResponded.emit(True)
        except grpc.RpcError as e:
            print(e)
            self.clientDeleteUserResponded.emit(False)

    @Slot(str)
    def mGetOtherUsers(self, query: str):
        try:
            req = pb2.GetOtherUsersRequest(
                user_id=self.user.id, query=query, limit=10, offset=0
            )
            res = self.stub.GetOtherUsers(req)
            print(f"GetOtherUsers :req: {ms(req)} :res: {ms(res)}")
            users = [User(user.user_id, user.username) for user in res.users]
            self.clientGetOtherUsersResponded.emit(True, users)
        except grpc.RpcError as e:
            print(e)
            self.clientGetOtherUsersResponded.emit(False, [])

    @Slot(int)
    def mCreateConversation(self, recv_user_id: int):
        try:
            req = pb2.CreateConversationRequest(
                user_id=self.user.id, other_user_id=recv_user_id
            )
            res = self.stub.CreateConversation(req)
            print(f"CreateConversation :req: {ms(req)} :res: {ms(res)}")
            self.conversation = Conversation(
                res.conversation_id,
                res.recv_user_id,
                res.recv_username,
            )
            self.clientCreateConversationResponded.emit(True, self.conversation)
        except grpc.RpcError as e:
            print(e)
            self.clientCreateConversationResponded.emit(False, None)

    @Slot()
    def mGetConversations(self):
        try:
            req = pb2.GetConversationsRequest(user_id=self.user.id)
            res = self.stub.GetConversations(req)
            print(f"GetConversations :req: {ms(req)} :res: {ms(res)}")
            conversations = [
                Conversation(
                    conversation.conversation_id,
                    conversation.recv_user_id,
                    conversation.recv_username,
                    conversation.unread_count,
                )
                for conversation in res.conversations
            ]
            self.clientGetConversationsResponded.emit(True, conversations)
        except grpc.RpcError as e:
            print(e)
            self.clientGetConversationsResponded.emit(False, [])

    @Slot(int)
    def mDeleteConversation(self, conversation_id: int):
        try:
            req = pb2.DeleteConversationRequest(conversation_id=conversation_id)
            res = self.stub.DeleteConversation(req)
            print(f"DeleteConversation :req: {ms(req)} :res: {ms(res)}")
            self.conversation = None
            self.clientDeleteConversationResponded.emit(True)
        except grpc.RpcError as e:
            print(e)
            self.clientDeleteConversationResponded.emit(False)

    @Slot(str)
    def mSendMessage(self, content: str):
        try:
            req = pb2.SendMessageRequest(
                conversation_id=self.conversation.id,
                send_user_id=self.user.id,
                content=content,
            )
            res = self.stub.SendMessage(req)
            print(f"SendMessage :req: {ms(req)} :res: {ms(res)}")
            self.clientSendMessageResponded.emit(True)
        except grpc.RpcError as e:
            print(e)
            self.clientSendMessageResponded.emit(False)

    @Slot()
    def mGetMessages(self):
        try:
            req = pb2.GetMessagesRequest(conversation_id=self.conversation.id)
            res = self.stub.GetMessages(req)
            print(f"GetMessages :req: {ms(req)} :res: {ms(res)}")
            messages = [
                Message(
                    message.message_id,
                    message.send_user_id,
                    message.is_read,
                    message.content,
                )
                for message in res.messages
            ]
            self.clientGetMessagesResponded.emit(True, messages)
        except grpc.RpcError as e:
            print(e)
            self.clientGetMessagesResponded.emit(False, [])

    @Slot(int)
    def mDeleteMessage(self, message_id: int):
        try:
            req = pb2.DeleteMessageRequest(
                conversation_id=self.conversation.id, message_id=message_id
            )
            res = self.stub.DeleteMessage(req)
            print(f"DeleteMessage :req: {ms(req)} :res: {ms(res)}")
            self.clientDeleteMessageResponded.emit(True)
        except grpc.RpcError as e:
            print(e)
            self.clientDeleteMessageResponded.emit(False)

    @Slot()
    def mCheck(self):
        print("check")
