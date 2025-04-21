#include "filter.hpp"

namespace mousedb {
namespace database {
namespace filter {
BloomFilter::BloomFilter(int size, int hash_count)
    : size(size), hash_count(hash_count), bit_array(size) {
}
}  // namespace filter
}  // namespace database
}  // namespace mousedb
