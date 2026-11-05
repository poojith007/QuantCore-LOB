#include "MatchingEngine.hpp"
#include <iostream>
#include <iomanip>
#include <vector>
#include <chrono>
#include <algorithm>
#include <numeric>
#include <random>

void printLevel2Book(const quant::OrderBook& book) {
    auto snapshot = book.getSnapshot(5);
    auto best_bid = book.bestBid();
    auto best_ask = book.bestAsk();
    auto spread   = book.spread();

    std::cout << "\n============================================================\n";
    std::cout << "                 LEVEL-2 ORDER BOOK DEPTH                   \n";
    std::cout << "============================================================\n";
    std::cout << std::left << std::setw(12) << "ORDERS" 
              << std::setw(14) << "BID QTY" 
              << std::setw(16) << "PRICE ($)" 
              << std::setw(14) << "ASK QTY" 
              << std::setw(10) << "ORDERS" << "\n";
    std::cout << "------------------------------------------------------------\n";

    // Print Asks (reversed so lowest ask is near the spread)
    for (auto it = snapshot.asks.rbegin(); it != snapshot.asks.rend(); ++it) {
        std::cout << std::left << std::setw(12) << ""
                  << std::setw(14) << ""
                  << "\033[31m" << std::setw(16) << std::fixed << std::setprecision(2) << quant::priceToDouble(it->price) << "\033[0m"
                  << std::setw(14) << it->volume
                  << std::setw(10) << it->order_count << "\n";
    }

    std::cout << "------------------------------------------------------------\n";
    if (spread) {
        std::cout << "  SPREAD: $" << std::fixed << std::setprecision(2) 
                  << quant::priceToDouble(*spread) 
                  << "  (Best Bid: $" << quant::priceToDouble(*best_bid)
                  << " | Best Ask: $" << quant::priceToDouble(*best_ask) << ")\n";
    } else {
        std::cout << "  SPREAD: N/A (One-sided or empty book)\n";
    }
    std::cout << "------------------------------------------------------------\n";

    // Print Bids (highest bid first)
    for (const auto& bid : snapshot.bids) {
        std::cout << std::left << std::setw(12) << bid.order_count
                  << std::setw(14) << bid.volume
                  << "\033[32m" << std::setw(16) << std::fixed << std::setprecision(2) << quant::priceToDouble(bid.price) << "\033[0m"
                  << std::setw(14) << ""
                  << std::setw(10) << "" << "\n";
    }
    std::cout << "============================================================\n\n";
}

void runQuickBenchmark(quant::MatchingEngine& engine, size_t num_orders = 200000) {
    std::cout << "\n>>> Starting Nanosecond Latency Benchmark (" << num_orders << " orders)... <<<\n";

    std::vector<uint64_t> latencies_ns;
    latencies_ns.reserve(num_orders);

    std::mt19937_64 rng(1337);
    std::uniform_int_distribution<int64_t> price_dist(100 * quant::PRICE_SCALE, 105 * quant::PRICE_SCALE);
    std::uniform_int_distribution<uint64_t> qty_dist(10, 500);
    std::uniform_int_distribution<int> side_dist(0, 1);

    // Warm-up phase
    for (size_t i = 1; i <= 5000; ++i) {
        quant::Side side = (i % 2 == 0) ? quant::Side::BUY : quant::Side::SELL;
        engine.submitLimitOrder(i, side, price_dist(rng), qty_dist(rng));
    }

    auto benchmark_start = std::chrono::high_resolution_clock::now();

    for (size_t i = 5001; i <= 5001 + num_orders; ++i) {
        quant::Side side = side_dist(rng) ? quant::Side::BUY : quant::Side::SELL;
        quant::Price price = price_dist(rng);
        quant::Quantity qty = qty_dist(rng);

        auto t0 = std::chrono::high_resolution_clock::now();
        engine.submitLimitOrder(i, side, price, qty);
        auto t1 = std::chrono::high_resolution_clock::now();

        latencies_ns.push_back(
            static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::nanoseconds>(t1 - t0).count())
        );
    }

    auto benchmark_end = std::chrono::high_resolution_clock::now();
    double total_sec = std::chrono::duration<double>(benchmark_end - benchmark_start).count();

    std::sort(latencies_ns.begin(), latencies_ns.end());

    auto p50  = latencies_ns[static_cast<size_t>(latencies_ns.size() * 0.50)];
    auto p90  = latencies_ns[static_cast<size_t>(latencies_ns.size() * 0.90)];
    auto p99  = latencies_ns[static_cast<size_t>(latencies_ns.size() * 0.99)];
    auto p999 = latencies_ns[static_cast<size_t>(latencies_ns.size() * 0.999)];
    double avg = std::accumulate(latencies_ns.begin(), latencies_ns.end(), 0.0) / latencies_ns.size();
    double mops = (static_cast<double>(num_orders) / total_sec) / 1'000'000.0;

    std::cout << "\n================ BENCHMARK RESULTS ================\n";
    std::cout << " Total Orders Measured : " << num_orders << "\n";
    std::cout << " Throughput            : " << std::fixed << std::setprecision(2) << mops << " Million ops/sec\n";
    std::cout << " Average Latency       : " << std::fixed << std::setprecision(1) << avg << " ns\n";
    std::cout << " Median (p50)          : " << p50 << " ns\n";
    std::cout << " 90th Percentile (p90) : " << p90 << " ns\n";
    std::cout << " 99th Percentile (p99) : " << p99 << " ns\n";
    std::cout << " 99.9th Percentile     : " << p999 << " ns\n";
    std::cout << " Min / Max Latency     : " << latencies_ns.front() << " ns / " << latencies_ns.back() << " ns\n";
    std::cout << "===================================================\n\n";
}

int main() {
    std::cout << "============================================================\n";
    std::cout << "        QuantCore-LOB: Ultra-Low Latency Matching Engine     \n";
    std::cout << "               Modern C++20 | Zero-Allocation Pool          \n";
    std::cout << "============================================================\n";

    quant::MatchingEngine engine;

    // Register real-time trade event callback
    engine.setTradeCallback([](const quant::Trade& trade) {
        std::cout << "\033[33m[TRADE EXECUTION]\033[0m "
                  << "Qty: " << std::setw(4) << trade.quantity 
                  << " @ $" << std::fixed << std::setprecision(2) << quant::priceToDouble(trade.price)
                  << " | Maker #" << trade.maker_order_id 
                  << " vs Taker #" << trade.taker_order_id
                  << " (" << quant::sideToString(trade.taker_side) << ")\n";
    });

    std::cout << "\n1. Seeding Initial Limit Orders into the Book...\n";
    // Place Bids
    engine.submitLimitOrder(101, quant::Side::BUY, quant::doubleToPrice(150.25), 100);
    engine.submitLimitOrder(102, quant::Side::BUY, quant::doubleToPrice(150.20), 250);
    engine.submitLimitOrder(103, quant::Side::BUY, quant::doubleToPrice(150.15), 500);
    engine.submitLimitOrder(104, quant::Side::BUY, quant::doubleToPrice(150.10), 1000);

    // Place Asks
    engine.submitLimitOrder(201, quant::Side::SELL, quant::doubleToPrice(150.35), 150);
    engine.submitLimitOrder(202, quant::Side::SELL, quant::doubleToPrice(150.40), 300);
    engine.submitLimitOrder(203, quant::Side::SELL, quant::doubleToPrice(150.50), 450);
    engine.submitLimitOrder(204, quant::Side::SELL, quant::doubleToPrice(150.60), 800);

    printLevel2Book(engine.orderBook());

    std::cout << "\n2. Submitting aggressive Market BUY Order (Qty: 200)...\n";
    std::cout << "   (Should completely fill Ask #201 @ $150.35 and partially fill Ask #202 @ $150.40)\n\n";
    engine.submitMarketOrder(301, quant::Side::BUY, 200);

    printLevel2Book(engine.orderBook());

    std::cout << "\n3. Submitting Limit SELL Order crossing spread (Qty: 400 @ $150.18)...\n";
    std::cout << "   (Matches Best Bids #101 @ $150.25 and #102 @ $150.20, remainder rests in book)\n\n";
    engine.submitLimitOrder(401, quant::Side::SELL, quant::doubleToPrice(150.18), 400);

    printLevel2Book(engine.orderBook());

    std::cout << "\n4. Canceling resting order #104...\n";
    bool canceled = engine.cancelOrder(104);
    std::cout << "   Order #104 cancellation status: " << (canceled ? "SUCCESS" : "FAILED") << "\n";

    printLevel2Book(engine.orderBook());

    // Run performance benchmark
    runQuickBenchmark(engine, 250000);

    return 0;
}
