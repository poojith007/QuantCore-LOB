#include "MatchingEngine.hpp"
#include <cassert>
#include <iostream>
#include <vector>

void testBasicLimitOrder() {
    std::cout << "[TEST] Running testBasicLimitOrder... ";
    quant::MatchingEngine engine;

    engine.submitLimitOrder(1, quant::Side::BUY, quant::doubleToPrice(100.0), 50);
    engine.submitLimitOrder(2, quant::Side::SELL, quant::doubleToPrice(101.0), 50);

    assert(engine.orderBook().bestBid() == quant::doubleToPrice(100.0));
    assert(engine.orderBook().bestAsk() == quant::doubleToPrice(101.0));
    assert(engine.orderBook().spread() == quant::doubleToPrice(1.0));
    assert(engine.orderBook().totalOrders() == 2);
    std::cout << "PASSED\n";
}

void testFifoPriceTimePriority() {
    std::cout << "[TEST] Running testFifoPriceTimePriority... ";
    quant::MatchingEngine engine;

    std::vector<quant::Trade> trades;
    engine.setTradeCallback([&trades](const quant::Trade& t) {
        trades.push_back(t);
    });

    // Place two sell orders at the exact same price: Order #1 placed before Order #2
    engine.submitLimitOrder(1, quant::Side::SELL, quant::doubleToPrice(105.0), 100);
    engine.submitLimitOrder(2, quant::Side::SELL, quant::doubleToPrice(105.0), 100);

    // Incoming buy order matches 150 shares
    engine.submitLimitOrder(3, quant::Side::BUY, quant::doubleToPrice(105.0), 150);

    // Order #1 should be completely filled first, then Order #2 partially filled
    assert(trades.size() == 2);
    assert(trades[0].maker_order_id == 1 && trades[0].quantity == 100);
    assert(trades[1].maker_order_id == 2 && trades[1].quantity == 50);

    // Order #1 must no longer be in book, Order #2 should remain with 50 shares
    assert(engine.orderBook().findOrder(1) == nullptr);
    quant::Order* remaining = engine.orderBook().findOrder(2);
    assert(remaining != nullptr);
    assert(remaining->remaining_quantity == 50);
    std::cout << "PASSED\n";
}

void testMarketOrderSweep() {
    std::cout << "[TEST] Running testMarketOrderSweep... ";
    quant::MatchingEngine engine;

    std::vector<quant::Trade> trades;
    engine.setTradeCallback([&trades](const quant::Trade& t) {
        trades.push_back(t);
    });

    // Asks at different price levels
    engine.submitLimitOrder(10, quant::Side::SELL, quant::doubleToPrice(50.0), 100);
    engine.submitLimitOrder(11, quant::Side::SELL, quant::doubleToPrice(51.0), 200);

    // Market buy of 250 shares
    engine.submitMarketOrder(20, quant::Side::BUY, 250);

    assert(trades.size() == 2);
    assert(trades[0].maker_order_id == 10 && trades[0].quantity == 100 && trades[0].price == quant::doubleToPrice(50.0));
    assert(trades[1].maker_order_id == 11 && trades[1].quantity == 150 && trades[1].price == quant::doubleToPrice(51.0));

    // Best ask should now be 51.0 with remaining 50 volume
    assert(engine.orderBook().bestAsk() == quant::doubleToPrice(51.0));
    quant::PriceLevel* level = engine.orderBook().getBestAskLevel();
    assert(level != nullptr);
    assert(level->totalVolume() == 50);
    std::cout << "PASSED\n";
}

void testOrderCancellation() {
    std::cout << "[TEST] Running testOrderCancellation... ";
    quant::MatchingEngine engine;

    engine.submitLimitOrder(1, quant::Side::BUY, quant::doubleToPrice(10.0), 100);
    engine.submitLimitOrder(2, quant::Side::BUY, quant::doubleToPrice(10.0), 200);

    assert(engine.orderBook().totalOrders() == 2);
    assert(engine.orderBook().bestBid() == quant::doubleToPrice(10.0));

    // Cancel first order
    bool res1 = engine.cancelOrder(1);
    assert(res1 == true);
    assert(engine.orderBook().totalOrders() == 1);
    assert(engine.orderBook().getBestBidLevel()->totalVolume() == 200);

    // Cancel non-existent order
    bool res_invalid = engine.cancelOrder(999);
    assert(res_invalid == false);

    // Cancel second order (cleans up price level entirely)
    bool res2 = engine.cancelOrder(2);
    assert(res2 == true);
    assert(engine.orderBook().totalOrders() == 0);
    assert(engine.orderBook().bestBid() == std::nullopt);
    std::cout << "PASSED\n";
}

int main() {
    std::cout << "\n=========================================\n";
    std::cout << "   RUNNING QUANTCORE-LOB UNIT TEST SUITE \n";
    std::cout << "=========================================\n";

    testBasicLimitOrder();
    testFifoPriceTimePriority();
    testMarketOrderSweep();
    testOrderCancellation();

    std::cout << "=========================================\n";
    std::cout << "      ALL UNIT TESTS PASSED SUCCESSFULLY! \n";
    std::cout << "=========================================\n\n";
    return 0;
}
