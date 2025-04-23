#pragma once

#include <span>
#include <string>

#include "arena.hpp"

namespace mousedb {
namespace memtable {
// Stores KV pairs, allowing for concurrent inserts.
// Example:
//    KVStore kvs(1024, 256, 64);
//    auto ptr = kvs.insert("key", "value");
//    auto [key, value] = kvs.convert(ptr);
class KVStore {
   public:
    using ptr_type = arena::ConcurrentArena::ptr_type;

    KVStore(size_t slab_size);

    auto insert(std::span<std::byte> key, std::span<std::byte> value)
        -> ptr_type;
    auto compare(ptr_type a, ptr_type b) -> int;
    auto convert(ptr_type ptr)
        -> std::pair<std::span<std::byte>, std::span<std::byte>>;

   private:
    arena::ConcurrentArena arena_;
};

class KVSkipList {
   public:
    KVSkipList();
    ~KVSkipList();

    void insert(const std::string &key, const std::string &value);
    std::string get(const std::string &key);
    void remove(const std::string &key);

   private:
    struct Node {
        std::string key;
        std::string value;
        Node *next;
    };

    Node *head_;
};

class MemTable {
   public:
    MemTable();
    ~MemTable();

    void insert(const std::string &key, const std::string &value);
    std::string get(const std::string &key);
    void remove(const std::string &key);
};
}  // namespace memtable
}  // namespace mousedb
