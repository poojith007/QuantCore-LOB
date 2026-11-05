#pragma once

#include <cstdint>
#include <string>
#include <chrono>

namespace quant {

using OrderId   = uint64_t;
using Price     = int64_t;   // Fixed-point price (e.g. $100.50 represented with 4 decimal places as 1005000)
using Quantity  = uint64_t;
using Timestamp = uint64_t;  // Nanoseconds since epoch

inline constexpr int64_t PRICE_SCALE = 10000; // 4 decimal places precision

inline Price doubleToPrice(double p) {
    return static_cast<Price>(p * PRICE_SCALE + (p >= 0 ? 0.5 : -0.5));
}

inline double priceToDouble(Price p) {
    return static_cast<double>(p) / PRICE_SCALE;
}

inline Timestamp currentTimestampNs() {
    return static_cast<Timestamp>(
        std::chrono::duration_cast<std::chrono::nanoseconds>(
            std::chrono::steady_clock::now().time_since_epoch()
        ).count()
    );
}

enum class Side : uint8_t {
    BUY  = 0,
    SELL = 1
};

inline const char* sideToString(Side side) {
    return side == Side::BUY ? "BUY" : "SELL";
}

enum class OrderType : uint8_t {
    LIMIT  = 0,
    MARKET = 1
};

inline const char* orderTypeToString(OrderType type) {
    return type == OrderType::LIMIT ? "LIMIT" : "MARKET";
}

struct Trade {
    OrderId   maker_order_id;
    OrderId   taker_order_id;
    Side      taker_side;
    Price     price;
    Quantity  quantity;
    Timestamp timestamp;
};

// Intrusive linked-list Order node to guarantee zero extra heap allocation
struct Order {
    OrderId   id{0};
    Price     price{0};
    Quantity  initial_quantity{0};
    Quantity  remaining_quantity{0};
    Side      side{Side::BUY};
    OrderType type{OrderType::LIMIT};
    Timestamp timestamp{0};

    // Intrusive pointers for O(1) insertion & removal inside price levels
    Order* prev{nullptr};
    Order* next{nullptr};

    [[nodiscard]] bool isFilled() const noexcept {
        return remaining_quantity == 0;
    }
};

} // namespace quant
