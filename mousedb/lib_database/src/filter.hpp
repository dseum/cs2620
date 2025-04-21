#pragma once

#include <boost/dynamic_bitset.hpp>
#include <vector>

namespace mousedb {
namespace database {
namespace filter {
class BloomFilter {
   public:
    BloomFilter(int size, int hash_count);
    void add(const std::string &item);
    bool contains(const std::string &item) const;

   private:
    int size;
    int hash_count;
    std::vector<bool> bit_array;

    int hash(const std::string &item, int seed) const;
};
}  // namespace filter
}  // namespace database
}  // namespace mousedb
