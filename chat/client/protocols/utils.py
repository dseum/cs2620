import struct
from enum import IntEnum, auto
from typing import Optional, Union, Any


class OperationCode(IntEnum):
    CLIENT_SIGNUP_USER = 0
    CLIENT_SIGNIN_USER = auto()
    CLIENT_SIGNOUT_USER = auto()
    CLIENT_DELETE_USER = auto()
    CLIENT_GET_OTHER_USERS = auto()
    CLIENT_CREATE_CONVERSATION = auto()
    CLIENT_GET_CONVERSATIONS = auto()
    CLIENT_DELETE_CONVERSATION = auto()
    CLIENT_SEND_MESSAGE = auto()
    CLIENT_GET_MESSAGES = auto()
    CLIENT_DELETE_MESSAGE = auto()
    SERVER_SEND_MESSAGE = auto()
    SERVER_UPDATE_UNREAD_COUNT = auto()


class StatusCode(IntEnum):
    SUCCESS = 0
    FAILURE = 1


class _Wrapper[F, T, *Ts]:
    """
    For serializing and deserializing data parameterized on [F, T, *Ts].

    F is the target type of the deserialized data.
    T is the type of the _value attribute (used for storage before serialization).
    Ts is the types of the additional values stored in _value.
    """

    _format: str
    _value: Optional[Union[tuple[T], tuple[T, *Ts]]]

    def __init__(self, value: Optional[T] = None):
        if value is None:
            self._value = None
            return
        self._value = (value,)

    @property
    def value(self) -> Union[tuple[T], tuple[T, *Ts]]:
        if self._value is None:
            raise ValueError("Value not set")
        return self._value

    @property
    def format(self) -> str:
        return self._format

    @classmethod
    def size(cls) -> int:
        return struct.calcsize("!" + cls._format)

    @classmethod
    def deserialize_from(cls, data: bytearray, offset: int) -> tuple[F, int]:
        format = "!" + cls._format
        new_offset = offset + struct.calcsize(format)
        return struct.unpack_from(format, data, offset)[0], new_offset


class BoolWrapper(_Wrapper[bool, bool]):
    _format = "?"


class Uint8Wrapper(_Wrapper[int, int]):
    _format = "B"


class Uint64Wrapper(_Wrapper[int, int]):
    _format = "Q"


class SizeWrapper(Uint64Wrapper):
    pass


class IdWrapper(Uint64Wrapper):
    pass


class StringWrapper(_Wrapper[str, int, bytearray]):
    _format = SizeWrapper._format

    def __init__(self, value: Optional[str]):
        if value is None:
            self._value = None
            return
        data = bytearray(value.encode("utf-8"))
        size = len(data)
        self._value = (size, data)
        self._format += str(size) + "s"

    @classmethod
    def deserialize_from(cls, data: bytearray, offset: int) -> tuple[str, int]:
        size, offset = SizeWrapper.deserialize_from(data, offset)
        new_offset = offset + size
        return struct.unpack_from(str(size) + "s", data, offset)[0].decode(
            "utf-8"
        ), new_offset


class OperationCodeWrapper(_Wrapper[OperationCode, int]):
    _format = "B"

    def __init__(self, value: Optional[OperationCode]):
        if value is None:
            self._value = None
            return
        self._value = (value.value,)

    @classmethod
    def deserialize_from(
        cls, data: bytearray, offset: int
    ) -> tuple[OperationCode, int]:
        value, new_offset = Uint8Wrapper.deserialize_from(data, offset)
        return OperationCode(value), new_offset


class StatusCodeWrapper(_Wrapper[StatusCode, int]):
    _format = "B"

    def __init__(self, value: Optional[StatusCode]):
        if value is None:
            self._value = None
            return
        self._value = (value.value,)

    @classmethod
    def deserialize_from(cls, data: bytearray, offset: int) -> tuple[StatusCode, int]:
        value, new_offset = Uint8Wrapper.deserialize_from(data, offset)
        return StatusCode(value), new_offset


def serialize(*args: _Wrapper) -> bytearray:
    assert len(args) > 0

    args_format = ""
    for arg in args:
        args_format += arg.format

    body_size = SizeWrapper(struct.calcsize("!" + args_format))

    values: list[Any] = [*body_size.value]
    format = "!" + body_size.format + args_format
    for arg in args:
        values.extend(arg.value)

    data = bytearray(struct.calcsize(format))
    struct.pack_into(format, data, 0, *values)

    return data
