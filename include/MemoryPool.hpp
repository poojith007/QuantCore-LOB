#pragma once

#include <vector>
#include <memory>
#include <cstddef>
#include <new>

namespace quant {

// Fast, pre-allocated memory pool designed for zero dynamic heap allocations on the hot path
template <typename T, size_t BlockSize = 65536>
class MemoryPool {
private:
    union Node {
        alignas(alignof(T)) std::byte storage[sizeof(T)];
        Node* next;
    };

    struct Block {
        std::unique_ptr<Node[]> data;
        explicit Block(size_t size) : data(std::make_unique<Node[]>(size)) {}
    };

    std::vector<Block> blocks_;
    Node* free_list_{nullptr};
    size_t allocated_count_{0};

    void allocateBlock() {
        blocks_.emplace_back(BlockSize);
        Node* block_data = blocks_.back().data.get();
        for (size_t i = 0; i < BlockSize - 1; ++i) {
            block_data[i].next = &block_data[i + 1];
        }
        block_data[BlockSize - 1].next = free_list_;
        free_list_ = &block_data[0];
    }

public:
    explicit MemoryPool(size_t initial_blocks = 4) {
        blocks_.reserve(initial_blocks * 2);
        for (size_t i = 0; i < initial_blocks; ++i) {
            allocateBlock();
        }
    }

    ~MemoryPool() = default;

    MemoryPool(const MemoryPool&) = delete;
    MemoryPool& operator=(const MemoryPool&) = delete;
    MemoryPool(MemoryPool&&) noexcept = default;
    MemoryPool& operator=(MemoryPool&&) noexcept = default;

    template <typename... Args>
    T* acquire(Args&&... args) {
        if (!free_list_) [[unlikely]] {
            allocateBlock();
        }

        Node* node = free_list_;
        free_list_ = free_list_->next;
        ++allocated_count_;

        T* ptr = reinterpret_cast<T*>(node->storage);
        return ::new (static_cast<void*>(ptr)) T(std::forward<Args>(args)...);
    }

    void release(T* ptr) noexcept {
        if (!ptr) return;

        ptr->~T();
        Node* node = reinterpret_cast<Node*>(ptr);
        node->next = free_list_;
        free_list_ = node;
        --allocated_count_;
    }

    [[nodiscard]] size_t activeCount() const noexcept {
        return allocated_count_;
    }

    [[nodiscard]] size_t totalCapacity() const noexcept {
        return blocks_.size() * BlockSize;
    }
};

} // namespace quant
