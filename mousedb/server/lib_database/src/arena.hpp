#pragma once

#include <atomic>
#include <deque>
#include <memory>
#include <vector>

class SpinMutex {
   public:
    SpinMutex() = default;
    SpinMutex(const SpinMutex &) = delete;
    SpinMutex &operator=(const SpinMutex &) = delete;

    auto lock() -> void {
        while (flag_.test_and_set(std::memory_order_acquire));
    }

    auto try_lock() -> bool {
        return !flag_.test_and_set(std::memory_order_acquire);
    }

    auto unlock() -> void {
        flag_.clear(std::memory_order_release);
    }

   private:
    std::atomic_flag flag_ = ATOMIC_FLAG_INIT;
};

namespace mousedb {
namespace arena {
class Arena {
   public:
    using ptr_type = std::byte *;

    explicit Arena(size_t slab_size);
    Arena(const Arena &) = delete;
    Arena &operator=(const Arena &) = delete;

    auto unused() const -> size_t;

    auto allocate(size_t size) -> ptr_type;
    auto allocate_slab(size_t size) -> ptr_type;

   private:
    const size_t slab_size_;
    std::deque<std::unique_ptr<std::byte[]>> slabs_;
    ptr_type active_slab_ = nullptr;
    size_t active_slab_unused_ = 0;
};

class ConcurrentArena {
   public:
    using ptr_type = std::byte *;

    explicit ConcurrentArena(size_t slab_size);
    ConcurrentArena(const ConcurrentArena &) = delete;
    ConcurrentArena &operator=(const ConcurrentArena &) = delete;

    auto unused() const -> size_t;

    auto allocate(size_t size) -> ptr_type;

   private:
    struct alignas(64) Shard {
        ptr_type begin = nullptr;
        std::atomic<size_t> unused = 0;
        SpinMutex mutex;
    };

    static thread_local size_t cpu_id_;

    const size_t num_cpus_;
    const size_t slab_size_;
    const size_t slab_slice_size_;

    Arena arena_;
    SpinMutex arena_mutex_;
    std::vector<Shard> shards_;
    std::atomic<size_t> unused_ = 0;

    inline auto update() -> void;
    inline auto get_shard(size_t cpu_id) const -> Shard *;
    inline auto reset_shard() -> Shard *;
};
}  // namespace arena
}  // namespace mousedb
