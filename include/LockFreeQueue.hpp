#pragma once

#include <atomic>
#include <vector>
#include <cstddef>
#include <new>

namespace quant {

// Hardware cache line size for aligning atomic pointers and avoiding false sharing
inline constexpr size_t CACHE_LINE_SIZE = 64;

// Single-Producer Single-Consumer (SPSC) Lock-Free Ring Buffer
template <typename T, size_t Capacity = 65536>
class LockFreeQueue {
    static_assert((Capacity & (Capacity - 1)) == 0, "Capacity must be a power of two");

private:
    std::vector<T> buffer_;
    static constexpr size_t MASK = Capacity - 1;

    // Separate head and tail across cache lines to avoid CPU cache bouncing / false sharing
    alignas(CACHE_LINE_SIZE) std::atomic<size_t> tail_{0}; // Producer writes here
    alignas(CACHE_LINE_SIZE) std::atomic<size_t> head_{0}; // Consumer reads from here

public:
    LockFreeQueue() : buffer_(Capacity) {}
    ~LockFreeQueue() = default;

    LockFreeQueue(const LockFreeQueue&) = delete;
    LockFreeQueue& operator=(const LockFreeQueue&) = delete;

    template <typename... Args>
    bool emplace(Args&&... args) {
        const size_t current_tail = tail_.load(std::memory_order_relaxed);
        const size_t current_head = head_.load(std::memory_order_acquire);

        if ((current_tail - current_head) >= Capacity) {
            return false; // Queue is full
        }

        buffer_[current_tail & MASK] = T(std::forward<Args>(args)...);
        tail_.store(current_tail + 1, std::memory_order_release);
        return true;
    }

    bool push(const T& item) {
        return emplace(item);
    }

    bool push(T&& item) {
        return emplace(std::move(item));
    }

    bool pop(T& item) {
        const size_t current_head = head_.load(std::memory_order_relaxed);
        const size_t current_tail = tail_.load(std::memory_order_acquire);

        if (current_head == current_tail) {
            return false; // Queue is empty
        }

        item = std::move(buffer_[current_head & MASK]);
        head_.store(current_head + 1, std::memory_order_release);
        return true;
    }

    [[nodiscard]] bool empty() const noexcept {
        return head_.load(std::memory_order_relaxed) == tail_.load(std::memory_order_relaxed);
    }

    [[nodiscard]] size_t size() const noexcept {
        const size_t head = head_.load(std::memory_order_relaxed);
        const size_t tail = tail_.load(std::memory_order_relaxed);
        return (tail >= head) ? (tail - head) : 0;
    }

    [[nodiscard]] static constexpr size_t capacity() noexcept {
        return Capacity;
    }
};

} // namespace quant
