#include "memtable.hpp"

#include <cstring>

size_t get_varint_size(uint64_t value) {
    size_t bits = 64 - std::countl_zero(value);
    return bits == 0 ? 1 : (bits + 6) / 7;
}

auto encode_varint(std::span<std::byte> data, uint64_t value) -> size_t {
    size_t i = 0;
    while (value > 0x7F) {
        data[i] = static_cast<std::byte>((value & 0x7F) | 0x80);
        value >>= 7;
        ++i;
    }
    data[i] = static_cast<std::byte>(value);
    return i + 1;
}

auto decode_varint(std::span<std::byte> data, uint64_t &value) -> size_t {
    size_t i = 0;
    value = 0;
    while (true) {
        value |= (static_cast<uint64_t>(data[i]) & 0x7F) << (i * 7);
        if ((static_cast<uint64_t>(data[i]) & 0x80) == 0) {
            break;
        }
        ++i;
    }
    return i + 1;
}

namespace mousedb {
namespace memtable {

KVStore::KVStore(size_t slab_size) : arena_(slab_size) {
}

auto KVStore::insert(std::span<std::byte> key, std::span<std::byte> value)
    -> ptr_type {
    const auto size_key = key.size();
    const auto size_value = value.size();
    const auto size_varint_key = get_varint_size(size_key);
    const auto size_varint_value = get_varint_size(size_value);
    const auto size =
        size_varint_key + size_varint_value + size_key + size_value;
    auto ptr = arena_.allocate(size);
    encode_varint({ptr, size_varint_key}, size_key);
    std::memcpy(ptr + size_varint_key, key.data(), size_key);
    encode_varint({ptr + size_varint_key + size_key, size_varint_value},
                  size_value);
    std::memcpy(ptr + size_varint_key + size_key + size_varint_value,
                value.data(), size_value);
    return ptr;
}

auto KVStore::compare(ptr_type a, ptr_type b) -> int {
    uint64_t size_a, size_b;
    size_t offset_a = decode_varint({a, sizeof(size_t)}, size_a);
    size_t offset_b = decode_varint({b, sizeof(size_t)}, size_b);
    int cmp = std::memcmp(a + offset_a, b + offset_b, std::min(size_a, size_b));
    if (cmp == 0) {
        if (size_a == size_b) {
            return 0;
        }
        return size_a < size_b ? -1 : 1;
    }
    return cmp;
}

auto KVStore::convert(ptr_type ptr)
    -> std::pair<std::span<std::byte>, std::span<std::byte>> {
    size_t offset = 0;
    uint64_t size_key;
    offset += decode_varint({ptr, sizeof(size_t)}, size_key);
    std::span<std::byte> key(ptr + offset, size_key);
    offset += size_key;
    uint64_t size_value;
    offset += decode_varint({ptr + offset, sizeof(size_t)}, size_value);
    std::span<std::byte> value(ptr + offset, size_value);
    return {key, value};
}

}  // namespace memtable
}  // namespace mousedb
