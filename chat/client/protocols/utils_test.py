import struct
import unittest

from .utils import (
    serialize,
    OperationCode,
    StatusCode,
    OperationCodeWrapper,
    StatusCodeWrapper,
    SizeWrapper,
    IdWrapper,
    StringWrapper,
)


class Test(unittest.TestCase):
    def test_operation_code(self):
        data = serialize(OperationCodeWrapper(OperationCode(0)))
        self.assertEqual(data, b"\x00\x00\x00\x00\x00\x00\x00\x01\x00")

    def test_status_code(self):
        data = serialize(StatusCodeWrapper(StatusCode(0)))
        self.assertEqual(data, b"\x00\x00\x00\x00\x00\x00\x00\x01\x00")

    def test_size(self):
        data = serialize(SizeWrapper(0))
        self.assertEqual(
            data, b"\x00\x00\x00\x00\x00\x00\x00\x08\x00\x00\x00\x00\x00\x00\x00\x00"
        )

    def test_id(self):
        data = serialize(IdWrapper(0))
        self.assertEqual(
            data, b"\x00\x00\x00\x00\x00\x00\x00\x08\x00\x00\x00\x00\x00\x00\x00\x00"
        )

    def test_string(self):
        data = serialize(StringWrapper("hello"))
        self.assertEqual(
            data,
            b"\x00\x00\x00\x00\x00\x00\x00\x0d\x00\x00\x00\x00\x00\x00\x00\x05hello",
        )

    def test_multiple(self):
        data = serialize(
            OperationCodeWrapper(OperationCode(0)),
            SizeWrapper(3),
            IdWrapper(4),
            StringWrapper("hello"),
        )
        self.assertEqual(
            data,
            b"\x00\x00\x00\x00\x00\x00\x00\x1e"
            b"\x00\x00\x00\x00\x00\x00\x00\x00"
            b"\x03"
            b"\x00\x00\x00\x00\x00\x00\x00\x04"
            b"\x00\x00\x00\x00\x00\x00\x00\x05hello",
        )

    def test_multiple_inversion(self):
        data = serialize(
            OperationCodeWrapper(OperationCode(0)),
            SizeWrapper(3),
            IdWrapper(4),
            StringWrapper("hello"),
        )
        offset = 0
        # First, deserialize the body size (which we ignore here)
        _, offset = SizeWrapper.deserialize_from(data, offset)
        cocw, offset = OperationCodeWrapper.deserialize_from(data, offset)
        self.assertEqual(cocw, OperationCode(0))
        sizew, offset = SizeWrapper.deserialize_from(data, offset)
        self.assertEqual(sizew, 3)
        iw, offset = IdWrapper.deserialize_from(data, offset)
        self.assertEqual(iw, 4)
        stringw, offset = StringWrapper.deserialize_from(data, offset)
        self.assertEqual(stringw, "hello")

    # Additional Tests

    def test_empty_string(self):
        """Test that an empty string is handled correctly."""
        original = ""
        data = serialize(StringWrapper(original))
        # Deserialize round-trip
        offset = 0
        # Read the body size (we ignore its value)
        _, offset = SizeWrapper.deserialize_from(data, offset)
        recovered, _ = StringWrapper.deserialize_from(data, offset)
        self.assertEqual(recovered, original)

    def test_unicode_string(self):
        """Test that a string with non-ASCII characters serializes/deserializes properly."""
        original = "héllö 🌍"
        data = serialize(StringWrapper(original))
        offset = 0
        # Read the body size
        _, offset = SizeWrapper.deserialize_from(data, offset)
        recovered, _ = StringWrapper.deserialize_from(data, offset)
        self.assertEqual(recovered, original)

    def test_value_not_set(self):
        """Test that accessing .value on a wrapper not set (None) raises a ValueError."""
        wrapper = StringWrapper(None)
        with self.assertRaises(ValueError):
            _ = wrapper.value

    def test_deserialize_short_data(self):
        """Test that deserializing from data that is too short raises a struct.error."""
        # Passing an empty bytearray should be too short for even a SizeWrapper.
        with self.assertRaises(struct.error):
            _ = SizeWrapper.deserialize_from(bytearray(), 0)

    def test_round_trip_operation_code(self):
        """Test round-trip serialization/deserialization for OperationCode."""
        original = OperationCode.CLIENT_SIGNIN_USER
        wrapper = OperationCodeWrapper(original)
        data = serialize(wrapper)
        offset = 0
        # Skip over the body size
        _, offset = SizeWrapper.deserialize_from(data, offset)
        recovered, _ = OperationCodeWrapper.deserialize_from(data, offset)
        self.assertEqual(recovered, original)

    def test_round_trip_status_code(self):
        """Test round-trip serialization/deserialization for StatusCode."""
        original = StatusCode.FAILURE
        wrapper = StatusCodeWrapper(original)
        data = serialize(wrapper)
        offset = 0
        _, offset = SizeWrapper.deserialize_from(data, offset)
        recovered, _ = StatusCodeWrapper.deserialize_from(data, offset)
        self.assertEqual(recovered, original)

    def test_round_trip_size(self):
        """Test round-trip for SizeWrapper with a typical value."""
        original = 123456789
        wrapper = SizeWrapper(original)
        data = serialize(wrapper)
        offset = 0
        _, offset = SizeWrapper.deserialize_from(data, offset)  # Skip the size header
        recovered, _ = SizeWrapper.deserialize_from(data, offset)
        self.assertEqual(recovered, original)

    def test_round_trip_id(self):
        """Test round-trip for IdWrapper with a typical value."""
        original = 987654321
        wrapper = IdWrapper(original)
        data = serialize(wrapper)
        offset = 0
        _, offset = SizeWrapper.deserialize_from(data, offset)
        recovered, _ = IdWrapper.deserialize_from(data, offset)
        self.assertEqual(recovered, original)
