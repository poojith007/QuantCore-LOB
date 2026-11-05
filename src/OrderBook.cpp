#include "OrderBook.hpp"

namespace quant {

OrderBook::OrderBook() : order_pool_(8) {
    orders_map_.reserve(100000);
}

OrderBook::~OrderBook() {
    for (auto& [id, order] : orders_map_) {
        order_pool_.release(order);
    }
    orders_map_.clear();
}

Order* OrderBook::insertOrder(OrderId id, Side side, Price price, Quantity quantity, Timestamp timestamp) {
    if (orders_map_.find(id) != orders_map_.end()) {
        return nullptr; // Duplicate order ID
    }

    Order* order = order_pool_.acquire();
    order->id = id;
    order->side = side;
    order->type = OrderType::LIMIT;
    order->price = price;
    order->initial_quantity = quantity;
    order->remaining_quantity = quantity;
    order->timestamp = timestamp;
    order->prev = nullptr;
    order->next = nullptr;

    if (side == Side::BUY) {
        auto [it, _] = bids_.try_emplace(price, price);
        it->second.addOrder(order);
    } else {
        auto [it, _] = asks_.try_emplace(price, price);
        it->second.addOrder(order);
    }

    orders_map_[id] = order;
    return order;
}

bool OrderBook::cancelOrder(OrderId id) {
    auto it = orders_map_.find(id);
    if (it == orders_map_.end()) {
        return false;
    }

    Order* order = it->second;
    if (order->side == Side::BUY) {
        auto level_it = bids_.find(order->price);
        if (level_it != bids_.end()) {
            level_it->second.removeOrder(order);
            if (level_it->second.empty()) {
                bids_.erase(level_it);
            }
        }
    } else {
        auto level_it = asks_.find(order->price);
        if (level_it != asks_.end()) {
            level_it->second.removeOrder(order);
            if (level_it->second.empty()) {
                asks_.erase(level_it);
            }
        }
    }

    orders_map_.erase(it);
    order_pool_.release(order);
    return true;
}

Order* OrderBook::findOrder(OrderId id) const {
    auto it = orders_map_.find(id);
    return (it != orders_map_.end()) ? it->second : nullptr;
}

std::optional<Price> OrderBook::bestBid() const {
    if (bids_.empty()) return std::nullopt;
    return bids_.begin()->first;
}

std::optional<Price> OrderBook::bestAsk() const {
    if (asks_.empty()) return std::nullopt;
    return asks_.begin()->first;
}

std::optional<Price> OrderBook::spread() const {
    auto b = bestBid();
    auto a = bestAsk();
    if (b && a) {
        return *a - *b;
    }
    return std::nullopt;
}

PriceLevel* OrderBook::getBestBidLevel() {
    if (bids_.empty()) return nullptr;
    return &(bids_.begin()->second);
}

PriceLevel* OrderBook::getBestAskLevel() {
    if (asks_.empty()) return nullptr;
    return &(asks_.begin()->second);
}

void OrderBook::removeEmptyBidLevel(Price price) {
    bids_.erase(price);
}

void OrderBook::removeEmptyAskLevel(Price price) {
    asks_.erase(price);
}

void OrderBook::freeOrder(Order* order) {
    if (!order) return;
    orders_map_.erase(order->id);
    order_pool_.release(order);
}

Level2Snapshot OrderBook::getSnapshot(size_t depth) const {
    Level2Snapshot snapshot;
    snapshot.timestamp = currentTimestampNs();

    snapshot.bids.reserve(depth);
    size_t count = 0;
    for (const auto& [price, level] : bids_) {
        if (count++ >= depth) break;
        snapshot.bids.push_back({price, level.totalVolume(), level.orderCount()});
    }

    snapshot.asks.reserve(depth);
    count = 0;
    for (const auto& [price, level] : asks_) {
        if (count++ >= depth) break;
        snapshot.asks.push_back({price, level.totalVolume(), level.orderCount()});
    }

    return snapshot;
}

} // namespace quant
