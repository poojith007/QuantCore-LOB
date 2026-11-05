#pragma once

#include "Types.hpp"
#include "PriceLevel.hpp"
#include "MemoryPool.hpp"
#include <map>
#include <unordered_map>
#include <vector>
#include <optional>

namespace quant {

struct Level2Entry {
    Price price{0};
    Quantity volume{0};
    uint32_t order_count{0};
};

struct Level2Snapshot {
    std::vector<Level2Entry> bids;
    std::vector<Level2Entry> asks;
    Timestamp timestamp{0};
};

class OrderBook {
private:
    // Bids sorted descending (highest bid first)
    std::map<Price, PriceLevel, std::greater<Price>> bids_;
    // Asks sorted ascending (lowest ask first)
    std::map<Price, PriceLevel, std::less<Price>> asks_;

    // Fast O(1) order lookup by ID
    std::unordered_map<OrderId, Order*> orders_map_;

    // Custom memory pool for zero-allocation order nodes
    MemoryPool<Order> order_pool_;

public:
    OrderBook();
    ~OrderBook();

    OrderBook(const OrderBook&) = delete;
    OrderBook& operator=(const OrderBook&) = delete;

    // Direct insertion of a resting limit order into the book
    Order* insertOrder(OrderId id, Side side, Price price, Quantity quantity, Timestamp timestamp);

    // Cancel resting order by ID
    bool cancelOrder(OrderId id);

    // Look up an order
    [[nodiscard]] Order* findOrder(OrderId id) const;

    // Current top of book
    [[nodiscard]] std::optional<Price> bestBid() const;
    [[nodiscard]] std::optional<Price> bestAsk() const;
    [[nodiscard]] std::optional<Price> spread() const;

    // Price level access for matching engine
    [[nodiscard]] PriceLevel* getBestBidLevel();
    [[nodiscard]] PriceLevel* getBestAskLevel();

    void removeEmptyBidLevel(Price price);
    void removeEmptyAskLevel(Price price);

    void freeOrder(Order* order);

    // L2 snapshot generation for market data dissemination
    [[nodiscard]] Level2Snapshot getSnapshot(size_t depth = 5) const;

    [[nodiscard]] size_t totalOrders() const noexcept { return orders_map_.size(); }
    [[nodiscard]] size_t bidLevelsCount() const noexcept { return bids_.size(); }
    [[nodiscard]] size_t askLevelsCount() const noexcept { return asks_.size(); }
};

} // namespace quant
