#pragma once

#include <cstdint>
#include <string>
#include <vector>

enum class OperationCode : uint8_t {
    CLIENT_SIGNUP_USER,
    CLIENT_SIGNIN_USER,
    CLIENT_SIGNOUT_USER,
    CLIENT_DELETE_USER,
    CLIENT_GET_OTHER_USERS,
    CLIENT_CREATE_CONVERSATION,
    CLIENT_GET_CONVERSATIONS,
    CLIENT_DELETE_CONVERSATION,
    CLIENT_SEND_MESSAGE,
    CLIENT_GET_MESSAGES,
    CLIENT_DELETE_MESSAGE,
    SERVER_SEND_MESSAGE,
    SERVER_UPDATE_UNREAD_COUNT
};

enum class StatusCode : uint8_t { SUCCESS = 0, FAILURE = 1 };

using Data = std::vector<uint8_t>;

using Bool = bool;

using Uint8 = uint8_t;

using Uint64 = uint64_t;

using Size = uint64_t;

using Id = uint64_t;

using String = std::string;

class BoolWrapper {
   public:
    using type = bool;

    static constexpr size_t size() { return sizeof(type); }

    static constexpr size_t size(const type&) { return sizeof(type); }

    static void serialize_to(Data& data, const type value);

    static std::pair<type, Data::size_type> deserialize_from(
        const Data& data, Data::size_type offset);
};

class Uint8Wrapper {
   public:
    using type = uint8_t;

    static constexpr size_t size() { return sizeof(type); }

    static constexpr size_t size(const type&) { return sizeof(type); }

    static void serialize_to(Data& data, const type value);

    static std::pair<type, Data::size_type> deserialize_from(
        const Data& data, Data::size_type offset);
};

class Uint64Wrapper {
   public:
    using type = uint64_t;

    static constexpr size_t size() { return sizeof(type); }

    static constexpr size_t size(const type&) { return sizeof(type); }

    static void serialize_to(Data& data, const type value);

    static std::pair<type, Data::size_type> deserialize_from(
        const Data& data, Data::size_type offset);
};

class SizeWrapper : public Uint64Wrapper {};

class IdWrapper : public Uint64Wrapper {};

class StringWrapper {
   public:
    using type = std::string;

    static size_t size();

    static size_t size(const type& value);

    // Serializes a string assuming it is UTF-8 encoded
    static void serialize_to(Data& data, const type& value);

    // Deserializes a string assuming it is UTF-8 encoded
    static std::pair<type, Data::size_type> deserialize_from(
        const Data& data, Data::size_type& offset);
};

class OperationCodeWrapper {
   public:
    using type = OperationCode;

    static constexpr size_t size() { return sizeof(type); }

    static constexpr size_t size(const type&) { return sizeof(type); }

    static void serialize_to(Data& data, const type value);

    static std::pair<type, Data::size_type> deserialize_from(
        const Data& data, Data::size_type offset);
};

class StatusCodeWrapper {
   public:
    using type = StatusCode;

    static constexpr size_t size() { return sizeof(type); }

    static constexpr size_t size(const type&) { return sizeof(type); }

    static void serialize_to(Data& data, const type value);

    static std::pair<type, Data::size_type> deserialize_from(
        const Data& data, Data::size_type offset);
};

// size is the number of bytes needed to serialize not including the body's
// initial size
Data initialize_serialization(size_t size);
