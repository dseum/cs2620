#pragma once
#include <atomic>
#include <chrono>
#include <cstdint>
#include <tuple>

/* ---------- Hybrid-Logical Clock record ---------------------------------- */
struct HLC {
    uint64_t physical_us;  // wall-clock, µs since UNIX epoch
    uint16_t logical;      // counter for same physical tick
    uint32_t node_id;      // tie-breaker

    /* partial ordering: “later” means ⟨phys,log,node⟩ lexicographically */
    friend constexpr auto operator<=>(const HLC &a, const HLC &b) {
        return std::tuple(a.physical_us, a.logical, a.node_id) <=>
               std::tuple(b.physical_us, b.logical, b.node_id);
    }
};

/* ---------- Thread-safe local clock -------------------------------------- */
class HybridClock {
   public:
    explicit HybridClock(uint32_t node_id) : node_id_(node_id) {
        auto now = phys_now();
        state_.store(pack(now, 0), std::memory_order_relaxed);
    }

    /* Call on every local generate/send event */
    [[nodiscard]] HLC now_send() {
        return advance(/*remote=*/std::nullopt);
    }

    /* Call once for each received remote HLC before you apply the mutation */
    [[nodiscard]] HLC recv_and_merge(const HLC &remote) {
        return advance(remote);
    }

   private:
    /* ------------ core algorithm (Kulkarni et al. 2014) ------------------ */
    [[nodiscard]] HLC advance(std::optional<HLC> remote) {
        uint64_t loop = state_.load(std::memory_order_relaxed);

        while (true) {
            uint64_t phys, old_phys;
            uint16_t log, old_log;
            unpack(loop, old_phys, old_log);

            phys = phys_now();
            uint16_t new_log = 0;

            if (remote) {
                /* incorporate remote clock */
                phys = std::max<uint64_t>(phys, remote->physical_us);
                phys = std::max<uint64_t>(phys, old_phys);
                bool same_local = phys == old_phys;
                bool same_remote = phys == remote->physical_us;

                if (same_local && same_remote)
                    new_log = std::max(old_log, remote->logical) + 1;
                else if (same_local)
                    new_log = old_log + 1;
                else if (same_remote)
                    new_log = remote->logical + 1;
                /* else new_log == 0 */
            } else {                                   /* local “send” event */
                if (phys < old_phys) phys = old_phys;  // clock retreat?
                new_log = (phys == old_phys) ? old_log + 1 : 0;
            }

            uint64_t packed = pack(phys, new_log);
            if (state_.compare_exchange_weak(loop, packed,
                                             std::memory_order_acq_rel,
                                             std::memory_order_relaxed)) {
                return HLC{phys, new_log, node_id_};
            }
            /* compare-exchange failed → loop contains latest state, retry */
        }
    }

    /* ------------ helpers ------------------------------------------------ */
    static inline uint64_t phys_now() {
        using namespace std::chrono;
        return duration_cast<microseconds>(
                   system_clock::now().time_since_epoch())
            .count();
    }

    static constexpr uint64_t pack(uint64_t phys, uint16_t log) {
        /* upper 48 bits = phys, lower 16 bits = logical */
        return (phys << 16) | log;
    }

    static constexpr void unpack(uint64_t x, uint64_t &phys, uint16_t &log) {
        phys = x >> 16;
        log = static_cast<uint16_t>(x & 0xFFFF);
    }

    const uint32_t node_id_;
    std::atomic<uint64_t> state_;  // packed ⟨phys,log⟩
};
