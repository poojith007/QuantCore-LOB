#pragma once

#include "Types.hpp"

namespace quant {

// Doubly-linked intrusive list of orders at a specific price point
class PriceLevel {
private:
    Price price_{0};
    Quantity total_volume_{0};
    uint32_t order_count_{0};

    Order* head_{nullptr};
    Order* tail_{nullptr};

public:
    PriceLevel() = default;
    explicit PriceLevel(Price price) : price_(price) {}

    [[nodiscard]] Price price() const noexcept { return price_; }
    [[nodiscard]] Quantity totalVolume() const noexcept { return total_volume_; }
    [[nodiscard]] uint32_t orderCount() const noexcept { return order_count_; }
    [[nodiscard]] bool empty() const noexcept { return order_count_ == 0; }
    [[nodiscard]] Order* head() const noexcept { return head_; }

    void addOrder(Order* order) noexcept {
        order->prev = tail_;
        order->next = nullptr;

        if (tail_) {
            tail_->next = order;
        } else {
            head_ = order;
        }
        tail_ = order;

        total_volume_ += order->remaining_quantity;
        ++order_count_;
    }

    void removeOrder(Order* order) noexcept {
        if (order->prev) {
            order->prev->next = order->next;
        } else {
            head_ = order->next;
        }

        if (order->next) {
            order->next->prev = order->prev;
        } else {
            tail_ = order->prev;
        }

        total_volume_ -= order->remaining_quantity;
        --order_count_;

        order->prev = nullptr;
        order->next = nullptr;
    }

    void reduceVolume(Quantity filled_qty) noexcept {
        total_volume_ = (total_volume_ >= filled_qty) ? (total_volume_ - filled_qty) : 0;
    }
};

} // namespace quant
