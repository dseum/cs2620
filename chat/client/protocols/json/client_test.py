import struct
import json
import unittest

# Import your request classes and any needed constants.
from .client import (
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
from ..utils import OperationCode, StatusCode, SizeWrapper

# Helper function to build an 8-byte header for a given body length.
def expected_size_header(body_length: int) -> bytes:
    return struct.pack("!Q", body_length)

class TestSignupUserRequestJSON(unittest.TestCase):
    def test_serialize(self):
        req = SignupUserRequest("username", "password")
        data = req.serialize()

        # Separate header (first 8 bytes) and JSON payload.
        header = data[:8]
        body = data[8:]
        expected_body = json.dumps({
            "operation_code": OperationCode.CLIENT_SIGNUP_USER,
            "username": "username",
            "password": "password",
        }, ensure_ascii=False).encode("utf-8")
        expected_header = expected_size_header(len(expected_body))
        self.assertEqual(header, expected_header)
        self.assertEqual(body, expected_body)

class TestSigninUserRequestJSON(unittest.TestCase):
    def test_serialize(self):
        req = SigninUserRequest("username", "password")
        data = req.serialize()

        header = data[:8]
        body = data[8:]
        expected_body = json.dumps({
            "operation_code": OperationCode.CLIENT_SIGNIN_USER,
            "username": "username",
            "password": "password",
        }, ensure_ascii=False).encode("utf-8")
        expected_header = expected_size_header(len(expected_body))
        self.assertEqual(header, expected_header)
        self.assertEqual(body, expected_body)

class TestSignoutUserRequestJSON(unittest.TestCase):
    def test_serialize(self):
        user_id = 123
        req = SignoutUserRequest(user_id)
        data = req.serialize()

        header = data[:8]
        body = data[8:]
        expected_body = json.dumps({
            "operation_code": OperationCode.CLIENT_SIGNOUT_USER,
            "user_id": user_id,
        }, ensure_ascii=False).encode("utf-8")
        expected_header = expected_size_header(len(expected_body))
        self.assertEqual(header, expected_header)
        self.assertEqual(body, expected_body)

class TestDeleteUserRequestJSON(unittest.TestCase):
    def test_serialize(self):
        user_id = 456
        password = "secret"
        req = DeleteUserRequest(user_id, password)
        data = req.serialize()

        header = data[:8]
        body = data[8:]
        expected_body = json.dumps({
            "operation_code": OperationCode.CLIENT_DELETE_USER,
            "user_id": user_id,
            "password": password,
        }, ensure_ascii=False).encode("utf-8")
        expected_header = expected_size_header(len(expected_body))
        self.assertEqual(header, expected_header)
        self.assertEqual(body, expected_body)

class TestGetOtherUsersRequestJSON(unittest.TestCase):
    def test_serialize(self):
        user_id = 1
        query = "test"
        limit = 10
        offset_val = 20
        req = GetOtherUsersRequest(user_id, query, limit, offset_val)
        data = req.serialize()

        header = data[:8]
        body = data[8:]
        expected_body = json.dumps({
            "operation_code": OperationCode.CLIENT_GET_OTHER_USERS,
            "user_id": user_id,
            "query": query,
            "limit": limit,
            "offset": offset_val,
        }, ensure_ascii=False).encode("utf-8")
        expected_header = expected_size_header(len(expected_body))
        self.assertEqual(header, expected_header)
        self.assertEqual(body, expected_body)

class TestCreateConversationRequestJSON(unittest.TestCase):
    def test_serialize(self):
        send_user_id = 100
        recv_user_id = 200
        req = CreateConversationRequest(send_user_id, recv_user_id)
        data = req.serialize()

        header = data[:8]
        body = data[8:]
        expected_body = json.dumps({
            "operation_code": OperationCode.CLIENT_CREATE_CONVERSATION,
            "send_user_id": send_user_id,
            "recv_user_id": recv_user_id,
        }, ensure_ascii=False).encode("utf-8")
        expected_header = expected_size_header(len(expected_body))
        self.assertEqual(header, expected_header)
        self.assertEqual(body, expected_body)

class TestGetConversationsRequestJSON(unittest.TestCase):
    def test_serialize(self):
        user_id = 300
        req = GetConversationsRequest(user_id)
        data = req.serialize()

        header = data[:8]
        body = data[8:]
        expected_body = json.dumps({
            "operation_code": OperationCode.CLIENT_GET_CONVERSATIONS,
            "user_id": user_id,
        }, ensure_ascii=False).encode("utf-8")
        expected_header = expected_size_header(len(expected_body))
        self.assertEqual(header, expected_header)
        self.assertEqual(body, expected_body)

class TestDeleteConversationRequestJSON(unittest.TestCase):
    def test_serialize(self):
        conversation_id = 400
        req = DeleteConversationRequest(conversation_id)
        data = req.serialize()

        header = data[:8]
        body = data[8:]
        expected_body = json.dumps({
            "operation_code": OperationCode.CLIENT_DELETE_CONVERSATION,
            "conversation_id": conversation_id,
        }, ensure_ascii=False).encode("utf-8")
        expected_header = expected_size_header(len(expected_body))
        self.assertEqual(header, expected_header)
        self.assertEqual(body, expected_body)

class TestSendMessageRequestJSON(unittest.TestCase):
    def test_serialize(self):
        conversation_id = 500
        sender_id = 600
        content = "hello"
        req = SendMessageRequest(conversation_id, sender_id, content)
        data = req.serialize()

        header = data[:8]
        body = data[8:]
        expected_body = json.dumps({
            "operation_code": OperationCode.CLIENT_SEND_MESSAGE,
            "conversation_id": conversation_id,
            "sender_id": sender_id,
            "content": content,
        }, ensure_ascii=False).encode("utf-8")
        expected_header = expected_size_header(len(expected_body))
        self.assertEqual(header, expected_header)
        self.assertEqual(body, expected_body)

class TestGetMessagesRequestJSON(unittest.TestCase):
    def test_serialize(self):
        conversation_id = 700
        req = GetMessagesRequest(conversation_id)
        data = req.serialize()

        header = data[:8]
        body = data[8:]
        expected_body = json.dumps({
            "operation_code": OperationCode.CLIENT_GET_MESSAGES,
            "conversation_id": conversation_id,
        }, ensure_ascii=False).encode("utf-8")
        expected_header = expected_size_header(len(expected_body))
        self.assertEqual(header, expected_header)
        self.assertEqual(body, expected_body)

class TestDeleteMessageRequestJSON(unittest.TestCase):
    def test_serialize(self):
        conversation_id = 900
        message_id = 1000
        req = DeleteMessageRequest(conversation_id, message_id)
        data = req.serialize()

        header = data[:8]
        body = data[8:]
        expected_body = json.dumps({
            "operation_code": OperationCode.CLIENT_DELETE_MESSAGE,
            "conversation_id": conversation_id,
            "message_id": message_id,
        }, ensure_ascii=False).encode("utf-8")
        expected_header = expected_size_header(len(expected_body))
        self.assertEqual(header, expected_header)
        self.assertEqual(body, expected_body)
