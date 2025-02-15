#include "utils.hpp"

#include <print>
#include <stdexcept>
#include <utility>

void BoolWrapper::serialize_to(Data& data, const type value) {
    data.emplace_back(static_cast<uint8_t>(value));
}

std::pair<BoolWrapper::type, Data::size_type> BoolWrapper::deserialize_from(
    const Data& data, Data::size_type offset) {
    type value = data[offset] == 1;
    offset += size();
    return {value, offset};
}

void Uint8Wrapper::serialize_to(Data& data, const type value) {
    data.emplace_back(value);
}

std::pair<Uint8Wrapper::type, Data::size_type> Uint8Wrapper::deserialize_from(
    const Data& data, Data::size_type offset) {
    type value = data[offset];
    offset += size();
    return {value, offset};
}

void Uint64Wrapper::serialize_to(Data& data, const type value) {
    data.push_back(static_cast<uint8_t>((value >> 56) & 0xFF));
    data.push_back(static_cast<uint8_t>((value >> 48) & 0xFF));
    data.push_back(static_cast<uint8_t>((value >> 40) & 0xFF));
    data.push_back(static_cast<uint8_t>((value >> 32) & 0xFF));
    data.push_back(static_cast<uint8_t>((value >> 24) & 0xFF));
    data.push_back(static_cast<uint8_t>((value >> 16) & 0xFF));
    data.push_back(static_cast<uint8_t>((value >> 8) & 0xFF));
    data.push_back(static_cast<uint8_t>(value & 0xFF));
}

std::pair<Uint64Wrapper::type, Data::size_type> Uint64Wrapper::deserialize_from(
    const Data& data, Data::size_type offset) {
    uint64_t value = static_cast<uint64_t>(data[offset]) << 56 |
                     static_cast<uint64_t>(data[offset + 1]) << 48 |
                     static_cast<uint64_t>(data[offset + 2]) << 40 |
                     static_cast<uint64_t>(data[offset + 3]) << 32 |
                     static_cast<uint64_t>(data[offset + 4]) << 24 |
                     static_cast<uint64_t>(data[offset + 5]) << 16 |
                     static_cast<uint64_t>(data[offset + 6]) << 8 |
                     static_cast<uint64_t>(data[offset + 7]);
    offset += size();
    return {value, offset};
}

void OperationCodeWrapper::serialize_to(Data& data, const type value) {
    data.emplace_back(std::to_underlying(value));
}

std::pair<OperationCodeWrapper::type, Data::size_type>
OperationCodeWrapper::deserialize_from(const Data& data,
                                       Data::size_type offset) {
    type value = static_cast<type>(data[offset]);
    offset += size();
    return {value, offset};
}

void StatusCodeWrapper::serialize_to(Data& data, const type value) {
    data.emplace_back(std::to_underlying(value));
}

std::pair<StatusCodeWrapper::type, Data::size_type>
StatusCodeWrapper::deserialize_from(const Data& data, Data::size_type offset) {
    type value = static_cast<type>(data[offset]);
    offset += size();
    return {value, offset};
}

size_t StringWrapper::size() {
    throw std::logic_error("not implemented as no meaning");
}

size_t StringWrapper::size(const type& value) {
    return SizeWrapper::size() + value.size();
}

void StringWrapper::serialize_to(Data& data, const StringWrapper::type& value) {
    uint64_t size = value.size();
    SizeWrapper::serialize_to(data, size);
    data.insert(data.end(), value.begin(), value.end());
}

std::pair<StringWrapper::type, Data::size_type> StringWrapper::deserialize_from(
    const Data& data, Data::size_type& offset) {
    auto [size, offset1] = SizeWrapper::deserialize_from(data, offset);
    std::string value(reinterpret_cast<const char*>(&data[offset1]), size);
    offset1 += size;
    return {value, offset1};
}

Data initialize_serialization(size_t payload_size) {
    // Allocate space for header + payload.
    Data data;
    data.reserve(SizeWrapper::size() + payload_size);
    SizeWrapper::serialize_to(data, payload_size);
    return data;
}
