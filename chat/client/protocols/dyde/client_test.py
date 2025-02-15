import struct
import unittest

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

# Helper functions to build expected wrappers.
# We assume the following:
# - All integer wrappers use big-endian unsigned 64-bit ("!Q").
# - OperationCodeWrapper uses a single unsigned byte ("B").
# - StringWrapper produces a field: 8-byte length (as Q) followed by the raw bytes.
#
# The serialize() function prepends the entire payload with its size
# (as a Q value, i.e. 8 bytes) computed from the sum of the sizes of the serialized wrappers.


def expected_size_header(body_length: int) -> bytes:
    return struct.pack("!Q", body_length)


def expected_uint8(value: int) -> bytes:
    return struct.pack("!B", value)


def expected_uint64(value: int) -> bytes:
    return struct.pack("!Q", value)


def expected_string(s: str) -> bytes:
    encoded = s.encode("utf-8")
    return struct.pack("!Q", len(encoded)) + encoded


class TestSignupUser(unittest.TestCase):
    def test_request(self):
        request = SignupUserRequest("username", "password")
        data = request.serialize()

        # Compute expected payload:
        # Operation code (SIGNUP_USER = 0) => 1 byte.
        op = expected_uint8(0)
        # "username" field: string wrapper produces 8-byte length then content.
        username_field = expected_string("username")
        # "password" field:
        password_field = expected_string("password")
        # Total payload is: op + username_field + password_field.
        payload = op + username_field + password_field
        expected = expected_size_header(len(payload)) + payload

        self.assertEqual(data, expected)


class TestSigninUser(unittest.TestCase):
    def test_request(self):
        request = SigninUserRequest("username", "password")
        data = request.serialize()

        # For SigninUserRequest, operation code = SIGNIN_USER (1).
        op = expected_uint8(1)
        username_field = expected_string("username")
        password_field = expected_string("password")
        payload = op + username_field + password_field
        expected = expected_size_header(len(payload)) + payload

        self.assertEqual(data, expected)


class TestSignoutUser(unittest.TestCase):
    def test_request(self):
        # Test with a sample user_id
        user_id = 123
        request = SignoutUserRequest(user_id)
        data = request.serialize()

        # Operation code for SIGNOUT_USER is 2.
        op = expected_uint8(2)
        id_field = expected_uint64(user_id)
        payload = op + id_field
        expected = expected_size_header(len(payload)) + payload

        self.assertEqual(data, expected)


class TestDeleteUser(unittest.TestCase):
    def test_request(self):
        user_id = 456
        password = "secret"
        request = DeleteUserRequest(user_id, password)
        data = request.serialize()

        # Operation code for DELETE_USER is 3.
        op = expected_uint8(3)
        id_field = expected_uint64(user_id)
        password_field = expected_string(password)
        payload = op + id_field + password_field
        expected = expected_size_header(len(payload)) + payload

        self.assertEqual(data, expected)


class TestGetOtherUsers(unittest.TestCase):
    def test_request(self):
        user_id = 1
        query = "test"
        limit = 10
        offset_val = 20
        request = GetOtherUsersRequest(user_id, query, limit, offset_val)
        data = request.serialize()

        # Operation code for GET_OTHER_USERS is 4.
        op = expected_uint8(4)
        id_field = expected_uint64(user_id)
        query_field = expected_string(query)
        limit_field = expected_uint64(limit)
        offset_field = expected_uint64(offset_val)
        payload = op + id_field + query_field + limit_field + offset_field
        expected = expected_size_header(len(payload)) + payload

        self.assertEqual(data, expected)


class TestCreateConversation(unittest.TestCase):
    def test_request(self):
        send_user_id = 100
        recv_user_id = 200
        request = CreateConversationRequest(send_user_id, recv_user_id)
        data = request.serialize()

        # Operation code for CREATE_CONVERSATION is 5.
        op = expected_uint8(5)
        send_id_field = expected_uint64(send_user_id)
        recv_id_field = expected_uint64(recv_user_id)
        payload = op + send_id_field + recv_id_field
        expected = expected_size_header(len(payload)) + payload

        self.assertEqual(data, expected)


class TestGetConversations(unittest.TestCase):
    def test_request(self):
        user_id = 300
        request = GetConversationsRequest(user_id)
        data = request.serialize()

        # Operation code for GET_CONVERSATIONS is 6.
        op = expected_uint8(6)
        id_field = expected_uint64(user_id)
        payload = op + id_field
        expected = expected_size_header(len(payload)) + payload

        self.assertEqual(data, expected)


class TestDeleteConversation(unittest.TestCase):
    def test_request(self):
        conversation_id = 400
        request = DeleteConversationRequest(conversation_id)
        data = request.serialize()

        # Operation code for DELETE_CONVERSATION is 7.
        op = expected_uint8(7)
        id_field = expected_uint64(conversation_id)
        payload = op + id_field
        expected = expected_size_header(len(payload)) + payload

        self.assertEqual(data, expected)


class TestSendMessage(unittest.TestCase):
    def test_request(self):
        conversation_id = 500
        sender_id = 600
        content = "hello"
        request = SendMessageRequest(conversation_id, sender_id, content)
        data = request.serialize()

        # Operation code for SEND_MESSAGE is 8.
        op = expected_uint8(8)
        conv_id_field = expected_uint64(conversation_id)
        sender_id_field = expected_uint64(sender_id)
        content_field = expected_string(content)
        payload = op + conv_id_field + sender_id_field + content_field
        expected = expected_size_header(len(payload)) + payload

        self.assertEqual(data, expected)


class TestGetMessages(unittest.TestCase):
    def test_request(self):
        conversation_id = 700
        request = GetMessagesRequest(conversation_id)
        data = request.serialize()

        # Operation code for GET_MESSAGES is 9.
        op = expected_uint8(9)
        conv_id_field = expected_uint64(conversation_id)
        payload = op + conv_id_field
        expected = expected_size_header(len(payload)) + payload

        self.assertEqual(data, expected)


class TestDeleteMessage(unittest.TestCase):
    def test_request(self):
        conversation_id = 900
        message_id = 1000
        request = DeleteMessageRequest(conversation_id, message_id)
        data = request.serialize()

        # Operation code for DELETE_MESSAGE is 10.
        op = expected_uint8(10)
        conv_id_field = expected_uint64(conversation_id)
        msg_id_field = expected_uint64(message_id)
        payload = op + conv_id_field + msg_id_field
        expected = expected_size_header(len(payload)) + payload

        self.assertEqual(data, expected)
