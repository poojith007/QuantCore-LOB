#include "MatchingEngine.hpp"
#include <algorithm>

namespace quant {

void MatchingEngine::processRequest(const OrderRequest& req) {
    ++total_orders_processed_;
    if (req.request_type == RequestType::CANCEL) {
        cancelOrder(req.id);
        return;
    }

    if (req.side == Side::BUY) {
        matchBuyOrder(req.id, req.order_type, req.price, req.quantity, req.timestamp);
    } else {
        matchSellOrder(req.id, req.order_type, req.price, req.quantity, req.timestamp);
    }
}

bool MatchingEngine::cancelOrder(OrderId id) {
    return order_book_.cancelOrder(id);
}

void MatchingEngine::submitLimitOrder(OrderId id, Side side, Price price, Quantity quantity, Timestamp timestamp) {
    OrderRequest req;
    req.request_type = RequestType::NEW;
    req.id = id;
    req.side = side;
    req.order_type = OrderType::LIMIT;
    req.price = price;
    req.quantity = quantity;
    req.timestamp = timestamp;
    processRequest(req);
}

void MatchingEngine::submitMarketOrder(OrderId id, Side side, Quantity quantity, Timestamp timestamp) {
    OrderRequest req;
    req.request_type = RequestType::NEW;
    req.id = id;
    req.side = side;
    req.order_type = OrderType::MARKET;
    req.price = 0;
    req.quantity = quantity;
    req.timestamp = timestamp;
    processRequest(req);
}

void MatchingEngine::matchBuyOrder(OrderId id, OrderType type, Price limit_price, Quantity quantity, Timestamp timestamp) {
    while (quantity > 0) {
        PriceLevel* best_ask_level = order_book_.getBestAskLevel();
        if (!best_ask_level) break;

        const Price match_price = best_ask_level->price();
        if (type == OrderType::LIMIT && match_price > limit_price) {
            break; // Spread not crossed
        }

        Order* maker = best_ask_level->head();
        while (maker && quantity > 0) {
            Order* next_maker = maker->next;
            const Quantity match_qty = std::min(quantity, maker->remaining_quantity);

            maker->remaining_quantity -= match_qty;
            best_ask_level->reduceVolume(match_qty);
            quantity -= match_qty;

            ++total_trades_executed_;
            total_volume_traded_ += match_qty;

            if (trade_callback_) {
                trade_callback_(Trade{
                    maker->id,
                    id,
                    Side::BUY,
                    match_price,
                    match_qty,
                    timestamp
                });
            }

            if (maker->isFilled()) {
                best_ask_level->removeOrder(maker);
                order_book_.freeOrder(maker);
            }

            maker = next_maker;
        }

        if (best_ask_level->empty()) {
            order_book_.removeEmptyAskLevel(match_price);
        }
    }

    // Resting limit order if not fully matched
    if (type == OrderType::LIMIT && quantity > 0) {
        order_book_.insertOrder(id, Side::BUY, limit_price, quantity, timestamp);
    }
}

void MatchingEngine::matchSellOrder(OrderId id, OrderType type, Price limit_price, Quantity quantity, Timestamp timestamp) {
    while (quantity > 0) {
        PriceLevel* best_bid_level = order_book_.getBestBidLevel();
        if (!best_bid_level) break;

        const Price match_price = best_bid_level->price();
        if (type == OrderType::LIMIT && match_price < limit_price) {
            break; // Spread not crossed
        }

        Order* maker = best_bid_level->head();
        while (maker && quantity > 0) {
            Order* next_maker = maker->next;
            const Quantity match_qty = std::min(quantity, maker->remaining_quantity);

            maker->remaining_quantity -= match_qty;
            best_bid_level->reduceVolume(match_qty);
            quantity -= match_qty;

            ++total_trades_executed_;
            total_volume_traded_ += match_qty;

            if (trade_callback_) {
                trade_callback_(Trade{
                    maker->id,
                    id,
                    Side::SELL,
                    match_price,
                    match_qty,
                    timestamp
                });
            }

            if (maker->isFilled()) {
                best_bid_level->removeOrder(maker);
                order_book_.freeOrder(maker);
            }

            maker = next_maker;
        }

        if (best_bid_level->empty()) {
            order_book_.removeEmptyBidLevel(match_price);
        }
    }

    // Resting limit order if not fully matched
    if (type == OrderType::LIMIT && quantity > 0) {
        order_book_.insertOrder(id, Side::SELL, limit_price, quantity, timestamp);
    }
}

} // namespace quant
