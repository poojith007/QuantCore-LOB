#pragma once

#include "OrderBook.hpp"
#include "LockFreeQueue.hpp"
#include <functional>
#include <vector>

namespace quant {

enum class RequestType : uint8_t {
    NEW    = 0,
    CANCEL = 1
};

struct OrderRequest {
    RequestType request_type{RequestType::NEW};
    OrderId     id{0};
    Side        side{Side::BUY};
    OrderType   order_type{OrderType::LIMIT};
    Price       price{0};
    Quantity    quantity{0};
    Timestamp   timestamp{0};
};

class MatchingEngine {
public:
    using TradeCallback = std::function<void(const Trade&)>;

private:
    OrderBook order_book_;
    TradeCallback trade_callback_{nullptr};

    uint64_t total_orders_processed_{0};
    uint64_t total_trades_executed_{0};
    uint64_t total_volume_traded_{0};

    void matchBuyOrder(OrderId id, OrderType type, Price limit_price, Quantity quantity, Timestamp timestamp);
    void matchSellOrder(OrderId id, OrderType type, Price limit_price, Quantity quantity, Timestamp timestamp);

public:
    MatchingEngine() = default;
    ~MatchingEngine() = default;

    void setTradeCallback(TradeCallback callback) {
        trade_callback_ = std::move(callback);
    }

    // Process a single order request synchronously
    void processRequest(const OrderRequest& req);

    // Cancel order by ID
    bool cancelOrder(OrderId id);

    // Direct API for convenient usage
    void submitLimitOrder(OrderId id, Side side, Price price, Quantity quantity, Timestamp timestamp = currentTimestampNs());
    void submitMarketOrder(OrderId id, Side side, Quantity quantity, Timestamp timestamp = currentTimestampNs());

    // Accessors
    [[nodiscard]] const OrderBook& orderBook() const noexcept { return order_book_; }
    [[nodiscard]] OrderBook& orderBook() noexcept { return order_book_; }

    [[nodiscard]] uint64_t totalOrdersProcessed() const noexcept { return total_orders_processed_; }
    [[nodiscard]] uint64_t totalTradesExecuted() const noexcept { return total_trades_executed_; }
    [[nodiscard]] uint64_t totalVolumeTraded() const noexcept { return total_volume_traded_; }
};

} // namespace quant
