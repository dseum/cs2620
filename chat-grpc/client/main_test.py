import unittest
import grpc

import gen.rpc.converse.converse_pb2 as pb2
from gen.rpc.converse.converse_pb2_grpc import ConverseServiceStub


class Backend:
    """
    A minimal 'Backend' class that calls each gRPC method synchronously.
    No threading, no PySide, just storing results in attributes:
        - self.user_id for SignIn/SignUp
        - self.last_other_users for GetOtherUsers
        - self.last_conversation_id etc. for CreateConversation
        - self.last_conversations for GetConversations
        - self.last_delete_conversation_success, etc.
        - self.last_message_id, self.last_messages...
    On failures, it either reverts to None/empty or keeps old values, depending on your preference.
    """

    def __init__(self, host="localhost", port=54100, stub=None):
        if stub is not None:
            self.stub = stub
        else:
            channel = grpc.insecure_channel(f"{host}:{port}")
            self.stub = ConverseServiceStub(channel)

        self.user_id = None
        self.last_other_users = []
        self.last_conversation_id = None
        self.last_conversation_recv_user_id = None
        self.last_conversation_recv_username = None
        self.last_conversations = []
        self.last_delete_conversation_success = False
        self.last_message_id = None
        self.last_messages = []
        self.last_delete_message_success = False

    def mSignUp(self, username: str, password: str):
        try:
            req = pb2.SignupUserRequest(username=username, password=password)
            res = self.stub.SignupUser(req)
            self.user_id = res.user_id
        except grpc.RpcError:
            # On failure, keep old user_id
            pass

    def mSignIn(self, username: str, password: str):
        try:
            req = pb2.SigninUserRequest(username=username, password=password)
            res = self.stub.SigninUser(req)
            self.user_id = res.user_id
        except grpc.RpcError:
            pass

    def mSignOut(self):
        if self.user_id is None:
            return
        try:
            req = pb2.SignoutUserRequest(user_id=self.user_id)
            self.stub.SignoutUser(req)
            self.user_id = None
        except grpc.RpcError:
            pass

    def mDeleteUser(self, password: str):
        if self.user_id is None:
            return
        try:
            req = pb2.DeleteUserRequest(user_id=self.user_id, password=password)
            self.stub.DeleteUser(req)
            self.user_id = None
        except grpc.RpcError:
            pass

    def mGetOtherUsers(self, query: str):
        try:
            req = pb2.GetOtherUsersRequest(
                user_id=self.user_id, query=query, limit=10, offset=0
            )
            res = self.stub.GetOtherUsers(req)
            self.last_other_users = res.users  # a list of pb2.User
        except grpc.RpcError:
            self.last_other_users = []

    def mCreateConversation(self, other_user_id: int):
        try:
            req = pb2.CreateConversationRequest(
                user_id=self.user_id, other_user_id=other_user_id
            )
            res = self.stub.CreateConversation(req)
            self.last_conversation_id = res.conversation_id
            self.last_conversation_recv_user_id = res.recv_user_id
            self.last_conversation_recv_username = res.recv_username
        except grpc.RpcError:
            self.last_conversation_id = None
            self.last_conversation_recv_user_id = None
            self.last_conversation_recv_username = None

    def mGetConversations(self):
        try:
            req = pb2.GetConversationsRequest(user_id=self.user_id)
            res = self.stub.GetConversations(req)
            self.last_conversations = res.conversations  # list of pb2.Conversation
        except grpc.RpcError:
            self.last_conversations = []

    def mDeleteConversation(self, conversation_id: int):
        try:
            req = pb2.DeleteConversationRequest(conversation_id=conversation_id)
            self.stub.DeleteConversation(req)
            self.last_delete_conversation_success = True
        except grpc.RpcError:
            self.last_delete_conversation_success = False

    def mSendMessage(self, conversation_id: int, content: str):
        try:
            req = pb2.SendMessageRequest(
                conversation_id=conversation_id,
                send_user_id=self.user_id,
                content=content,
            )
            res = self.stub.SendMessage(req)
            self.last_message_id = res.message_id
        except grpc.RpcError:
            self.last_message_id = None

    def mGetMessages(self, conversation_id: int):
        try:
            req = pb2.GetMessagesRequest(conversation_id=conversation_id)
            res = self.stub.GetMessages(req)
            self.last_messages = res.messages  # list of pb2.Message
        except grpc.RpcError:
            self.last_messages = []

    def mDeleteMessage(self, conversation_id: int, message_id: int):
        """
        We'll store success/failure in self.last_delete_message_success.
        """
        try:
            req = pb2.DeleteMessageRequest(
                conversation_id=conversation_id, message_id=message_id
            )
            self.stub.DeleteMessage(req)
            self.last_delete_message_success = True
        except grpc.RpcError:
            self.last_delete_message_success = False

class DummyRpcError(grpc.RpcError):
    def __init__(self, message):
        super().__init__()
        self._message = message

    def __str__(self):
        return self._message


class FakeStub:
    def __init__(self, responses):
        self.responses = responses

    def SignupUser(self, req):
        r = self.responses.get("SignupUser")
        if isinstance(r, Exception):
            raise r
        return r

    def SigninUser(self, req):
        r = self.responses.get("SigninUser")
        if isinstance(r, Exception):
            raise r
        return r

    def SignoutUser(self, req):
        r = self.responses.get("SignoutUser")
        if isinstance(r, Exception):
            raise r
        return r

    def DeleteUser(self, req):
        r = self.responses.get("DeleteUser")
        if isinstance(r, Exception):
            raise r
        return r

    def GetOtherUsers(self, req):
        r = self.responses.get("GetOtherUsers")
        if isinstance(r, Exception):
            raise r
        return r

    def CreateConversation(self, req):
        r = self.responses.get("CreateConversation")
        if isinstance(r, Exception):
            raise r
        return r

    def GetConversations(self, req):
        r = self.responses.get("GetConversations")
        if isinstance(r, Exception):
            raise r
        return r

    def DeleteConversation(self, req):
        r = self.responses.get("DeleteConversation")
        if isinstance(r, Exception):
            raise r
        return r

    def SendMessage(self, req):
        r = self.responses.get("SendMessage")
        if isinstance(r, Exception):
            raise r
        return r

    def GetMessages(self, req):
        r = self.responses.get("GetMessages")
        if isinstance(r, Exception):
            raise r
        return r

    def DeleteMessage(self, req):
        r = self.responses.get("DeleteMessage")
        if isinstance(r, Exception):
            raise r
        return r

class TestBackend(unittest.TestCase):
    def test_mSignUp_success(self):
        success_resp = pb2.SignupUserResponse(user_id=101)
        fake_stub = FakeStub({"SignupUser": success_resp})
        backend = Backend(stub=fake_stub)
        backend.mSignUp("alice", "password")
        self.assertEqual(backend.user_id, 101)

    def test_mSignUp_failure(self):
        error_resp = DummyRpcError("Signup failed")
        fake_stub = FakeStub({"SignupUser": error_resp})
        backend = Backend(stub=fake_stub)
        backend.mSignUp("bob", "badpass")
        self.assertIsNone(backend.user_id)

    def test_mSignIn_success(self):
        success_resp = pb2.SigninUserResponse(user_id=202)
        fake_stub = FakeStub({"SigninUser": success_resp})
        backend = Backend(stub=fake_stub)
        backend.mSignIn("carol", "pass")
        self.assertEqual(backend.user_id, 202)

    def test_mSignIn_failure(self):
        error_resp = DummyRpcError("SignIn failed")
        fake_stub = FakeStub({"SigninUser": error_resp})
        backend = Backend(stub=fake_stub)
        backend.mSignIn("eve", "wrong")
        self.assertIsNone(backend.user_id)

    def test_mSignOut_success(self):
        # Start with user_id set
        success_resp = pb2.SignoutUserResponse()
        fake_stub = FakeStub({"SignoutUser": success_resp})
        backend = Backend(stub=fake_stub)
        backend.user_id = 300
        backend.mSignOut()
        self.assertIsNone(backend.user_id)

    def test_mSignOut_failure(self):
        error_resp = DummyRpcError("SignOut failed")
        fake_stub = FakeStub({"SignoutUser": error_resp})
        backend = Backend(stub=fake_stub)
        backend.user_id = 300
        backend.mSignOut()
        # user_id remains on failure
        self.assertEqual(backend.user_id, 300)

    def test_mDeleteUser_success(self):
        success_resp = pb2.DeleteUserResponse()
        fake_stub = FakeStub({"DeleteUser": success_resp})
        backend = Backend(stub=fake_stub)
        backend.user_id = 400
        backend.mDeleteUser("secret")
        self.assertIsNone(backend.user_id)

    def test_mDeleteUser_failure(self):
        error_resp = DummyRpcError("DeleteUser failed")
        fake_stub = FakeStub({"DeleteUser": error_resp})
        backend = Backend(stub=fake_stub)
        backend.user_id = 500
        backend.mDeleteUser("wrongpass")
        self.assertEqual(backend.user_id, 500)

    def test_mGetOtherUsers_success(self):
        user1 = pb2.User(user_id=1, username="alice")
        user2 = pb2.User(user_id=2, username="bob")
        success_resp = pb2.GetOtherUsersResponse(users=[user1, user2])
        fake_stub = FakeStub({"GetOtherUsers": success_resp})
        backend = Backend(stub=fake_stub)
        backend.user_id = 999  # must be logged in for "GetOtherUsers"
        backend.mGetOtherUsers("test")
        # check that we stored them
        self.assertEqual(len(backend.last_other_users), 2)
        self.assertEqual(backend.last_other_users[0].username, "alice")

    def test_mGetOtherUsers_failure(self):
        error_resp = DummyRpcError("GetOtherUsers failed")
        fake_stub = FakeStub({"GetOtherUsers": error_resp})
        backend = Backend(stub=fake_stub)
        backend.user_id = 999
        backend.mGetOtherUsers("test")
        self.assertEqual(backend.last_other_users, [])

    def test_mCreateConversation_success(self):
        success_resp = pb2.CreateConversationResponse(
            conversation_id=123,
            recv_user_id=456,
            recv_username="buddy",
        )
        fake_stub = FakeStub({"CreateConversation": success_resp})
        backend = Backend(stub=fake_stub)
        backend.user_id = 999
        backend.mCreateConversation(456)
        self.assertEqual(backend.last_conversation_id, 123)
        self.assertEqual(backend.last_conversation_recv_username, "buddy")

    def test_mCreateConversation_failure(self):
        error_resp = DummyRpcError("CreateConversation error")
        fake_stub = FakeStub({"CreateConversation": error_resp})
        backend = Backend(stub=fake_stub)
        backend.user_id = 999
        # set them to something to ensure they get cleared on failure
        backend.last_conversation_id = 111
        backend.last_conversation_recv_username = "old"
        backend.mCreateConversation(456)
        self.assertIsNone(backend.last_conversation_id)
        self.assertIsNone(backend.last_conversation_recv_username)

    def test_mGetConversations_success(self):
        conv1 = pb2.Conversation(conversation_id=10, recv_user_id=1,
                                 recv_username="alice", unread_count=2)
        conv2 = pb2.Conversation(conversation_id=20, recv_user_id=2,
                                 recv_username="bob", unread_count=0)
        success_resp = pb2.GetConversationsResponse(conversations=[conv1, conv2])
        fake_stub = FakeStub({"GetConversations": success_resp})
        backend = Backend(stub=fake_stub)
        backend.user_id = 999
        backend.mGetConversations()
        self.assertEqual(len(backend.last_conversations), 2)
        self.assertEqual(backend.last_conversations[0].conversation_id, 10)

    def test_mGetConversations_failure(self):
        error_resp = DummyRpcError("GetConversations error")
        fake_stub = FakeStub({"GetConversations": error_resp})
        backend = Backend(stub=fake_stub)
        backend.user_id = 999
        # set a prior list so we can confirm it changes to empty on failure
        backend.last_conversations = [pb2.Conversation(conversation_id=111)]
        backend.mGetConversations()
        self.assertEqual(backend.last_conversations, [])

    def test_mDeleteConversation_success(self):
        success_resp = pb2.DeleteConversationResponse()
        fake_stub = FakeStub({"DeleteConversation": success_resp})
        backend = Backend(stub=fake_stub)
        backend.user_id = 999
        backend.mDeleteConversation(111)
        self.assertTrue(backend.last_delete_conversation_success)

    def test_mDeleteConversation_failure(self):
        error_resp = DummyRpcError("DeleteConversation error")
        fake_stub = FakeStub({"DeleteConversation": error_resp})
        backend = Backend(stub=fake_stub)
        backend.user_id = 999
        backend.last_delete_conversation_success = True
        backend.mDeleteConversation(111)
        self.assertFalse(backend.last_delete_conversation_success)

    def test_mSendMessage_success(self):
        success_resp = pb2.SendMessageResponse(message_id=555)
        fake_stub = FakeStub({"SendMessage": success_resp})
        backend = Backend(stub=fake_stub)
        backend.user_id = 999
        backend.mSendMessage(123, "hello world")
        self.assertEqual(backend.last_message_id, 555)

    def test_mSendMessage_failure(self):
        error_resp = DummyRpcError("SendMessage error")
        fake_stub = FakeStub({"SendMessage": error_resp})
        backend = Backend(stub=fake_stub)
        backend.user_id = 999
        backend.last_message_id = 321
        backend.mSendMessage(123, "fail now")
        self.assertIsNone(backend.last_message_id)

    def test_mGetMessages_success(self):
        msg1 = pb2.Message(message_id=1, send_user_id=999,
                           is_read=False, content="hi")
        msg2 = pb2.Message(message_id=2, send_user_id=456,
                           is_read=True, content="response")
        success_resp = pb2.GetMessagesResponse(messages=[msg1, msg2])
        fake_stub = FakeStub({"GetMessages": success_resp})
        backend = Backend(stub=fake_stub)
        backend.user_id = 999
        backend.mGetMessages(123)
        self.assertEqual(len(backend.last_messages), 2)
        self.assertEqual(backend.last_messages[1].content, "response")

    def test_mGetMessages_failure(self):
        error_resp = DummyRpcError("GetMessages error")
        fake_stub = FakeStub({"GetMessages": error_resp})
        backend = Backend(stub=fake_stub)
        backend.user_id = 999
        backend.last_messages = [pb2.Message(message_id=99)]
        backend.mGetMessages(123)
        self.assertEqual(backend.last_messages, [])

    def test_mDeleteMessage_success(self):
        success_resp = pb2.DeleteMessageResponse()
        fake_stub = FakeStub({"DeleteMessage": success_resp})
        backend = Backend(stub=fake_stub)
        backend.user_id = 999
        backend.mDeleteMessage(123, 9999)
        self.assertTrue(backend.last_delete_message_success)

    def test_mDeleteMessage_failure(self):
        error_resp = DummyRpcError("DeleteMessage error")
        fake_stub = FakeStub({"DeleteMessage": error_resp})
        backend = Backend(stub=fake_stub)
        backend.user_id = 999
        backend.last_delete_message_success = True
        backend.mDeleteMessage(123, 9999)
        self.assertFalse(backend.last_delete_message_success)
