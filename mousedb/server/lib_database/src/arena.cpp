#include "arena.hpp"

#include <sched.h>
#include <unistd.h>

#include <algorithm>
#include <mutex>
#include <thread>

inline static auto optimize_slab_size(size_t slab_size) -> size_t {
    slab_size = std::max(slab_size, static_cast<size_t>(4096));
    slab_size = std::min(slab_size, static_cast<size_t>(2u << 30));
    size_t align_unit = alignof(std::max_align_t);
    if (slab_size % align_unit != 0) {
        slab_size = (1 + slab_size / align_unit) * align_unit;
    }
    return slab_size;
}

namespace mousedb {
namespace arena {

Arena::Arena(size_t slab_size) : slab_size_(optimize_slab_size(slab_size)) {
}

auto Arena::allocate(size_t size) -> ptr_type {
    if (size >= slab_size_) {
        return allocate_slab(size);
    }
    if (size > active_slab_unused_) {
        active_slab_ = allocate_slab(slab_size_);
        active_slab_unused_ = slab_size_ - size;
        return active_slab_;
    }
    ptr_type ptr = active_slab_ + (slab_size_ - active_slab_unused_);
    active_slab_unused_ -= size;
    return ptr;
}

auto Arena::allocate_slab(size_t size) -> ptr_type {
    ptr_type slab = new std::byte[size];
    slabs_.emplace_back(slab);
    return slab;
}

auto Arena::unused() const -> size_t {
    return active_slab_unused_;
}

thread_local size_t ConcurrentArena::cpu_id_ = 0;

static auto round_up_to_power_of_two(size_t n) -> size_t {
    if (n == 0) return 1;
    --n;
    for (size_t i = 1; i < sizeof(size_t) * 8; i *= 2) {
        n |= n >> i;
    }
    return n + 1;
}

ConcurrentArena::ConcurrentArena(size_t slab_size)
    : num_cpus_(std::thread::hardware_concurrency()),
      slab_size_(slab_size),
      slab_slice_size_(
          std::min(static_cast<size_t>(128 * 1024), slab_size_ / 8)),
      arena_(slab_size_),
      shards_(round_up_to_power_of_two(num_cpus_)) {
}

auto ConcurrentArena::unused() const -> size_t {
    return unused_.load(std::memory_order_relaxed);
}

auto ConcurrentArena::allocate(size_t size) -> ptr_type {
    size_t cpu_id;
    std::unique_lock arena_lock(arena_mutex_, std::defer_lock);
    if (size > slab_slice_size_ / 4 ||
        ((cpu_id = cpu_id_) == 0 && get_shard(0)->unused == 0 &&
         arena_lock.try_lock())) {
        if (!arena_lock.owns_lock()) {
            arena_lock.lock();
        }
        ptr_type rv = arena_.allocate(size);
        update();
        return rv;
    }
    Shard *shard = get_shard(cpu_id);
    if (!shard->mutex.try_lock()) {
        shard = reset_shard();
        shard->mutex.lock();
    }
    std::unique_lock shard_lock(shard->mutex, std::adopt_lock);
    size_t shard_unused = shard->unused.load(std::memory_order_relaxed);
    if (shard_unused < size) {
        std::scoped_lock reloaded_arena_lock(arena_mutex_);
        size_t unused = unused_.load(std::memory_order_relaxed);
        shard_unused =
            unused >= slab_slice_size_ / 2 && unused < slab_slice_size_ * 2
                ? unused
                : slab_slice_size_;
        shard->begin = arena_.allocate(shard_unused);
        update();
    }
    shard->unused.store(shard_unused - size, std::memory_order_relaxed);
    ptr_type rv = shard->begin;
    shard->begin += size;
    return rv;
}

inline auto ConcurrentArena::update() -> void {
    unused_.store(arena_.unused(), std::memory_order_relaxed);
}

inline auto ConcurrentArena::get_shard(size_t cpu_id) const -> Shard * {
    return const_cast<Shard *>(&shards_[cpu_id & (shards_.size() - 1)]);
}

inline auto ConcurrentArena::reset_shard() -> Shard * {
    size_t cpu_id = sched_getcpu();
    if (cpu_id == static_cast<size_t>(-1)) [[unlikely]] {
        cpu_id = std::hash<std::thread::id>()(std::this_thread::get_id());
    }
    cpu_id_ = cpu_id | shards_.size();
    return get_shard(cpu_id);
}

}  // namespace arena
}  // namespace mousedb
