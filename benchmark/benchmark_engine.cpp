#include "MatchingEngine.hpp"
#include "LockFreeQueue.hpp"
#include <iostream>
#include <vector>
#include <chrono>
#include <algorithm>
#include <numeric>
#include <random>
#include <thread>
#include <atomic>
#include <iomanip>

struct BenchmarkStats {
    uint64_t total_ops{0};
    double total_time_sec{0.0};
    double mops{0.0};
    double avg_ns{0.0};
    uint64_t min_ns{0};
    uint64_t p50_ns{0};
    uint64_t p90_ns{0};
    uint64_t p99_ns{0};
    uint64_t p999_ns{0};
    uint64_t max_ns{0};
};

BenchmarkStats computeStats(std::vector<uint64_t>& latencies, double total_sec) {
    BenchmarkStats stats;
    stats.total_ops = latencies.size();
    stats.total_time_sec = total_sec;
    stats.mops = (static_cast<double>(stats.total_ops) / total_sec) / 1'000'000.0;

    std::sort(latencies.begin(), latencies.end());
    stats.min_ns  = latencies.front();
    stats.max_ns  = latencies.back();
    stats.p50_ns  = latencies[static_cast<size_t>(latencies.size() * 0.50)];
    stats.p90_ns  = latencies[static_cast<size_t>(latencies.size() * 0.90)];
    stats.p99_ns  = latencies[static_cast<size_t>(latencies.size() * 0.99)];
    stats.p999_ns = latencies[static_cast<size_t>(latencies.size() * 0.999)];
    stats.avg_ns  = std::accumulate(latencies.begin(), latencies.end(), 0.0) / latencies.size();
    return stats;
}

void printStats(const std::string& title, const BenchmarkStats& stats) {
    std::cout << "\n======================================================\n";
    std::cout << "  " << title << "\n";
    std::cout << "======================================================\n";
    std::cout << "  Operations Processed : " << stats.total_ops << "\n";
    std::cout << "  Elapsed Time         : " << std::fixed << std::setprecision(4) << stats.total_time_sec << " s\n";
    std::cout << "  Throughput           : " << std::fixed << std::setprecision(2) << stats.mops << " Million ops/sec\n";
    std::cout << "  Average Latency      : " << std::fixed << std::setprecision(1) << stats.avg_ns << " ns\n";
    std::cout << "  Min Latency          : " << stats.min_ns << " ns\n";
    std::cout << "  Median (p50)         : " << stats.p50_ns << " ns\n";
    std::cout << "  90th Percentile(p90) : " << stats.p90_ns << " ns\n";
    std::cout << "  99th Percentile(p99) : " << stats.p99_ns << " ns\n";
    std::cout << "  99.9th Perc. (p99.9) : " << stats.p999_ns << " ns\n";
    std::cout << "  Max Latency          : " << stats.max_ns << " ns\n";
    std::cout << "======================================================\n\n";
}

int main() {
    std::cout << "======================================================\n";
    std::cout << "   QuantCore-LOB High-Precision Benchmark Suite       \n";
    std::cout << "======================================================\n";

    constexpr size_t NUM_OPERATIONS = 1000000;
    quant::MatchingEngine engine;

    std::mt19937_64 rng(42);
    std::uniform_int_distribution<int64_t> price_dist(90 * quant::PRICE_SCALE, 110 * quant::PRICE_SCALE);
    std::uniform_int_distribution<uint64_t> qty_dist(10, 1000);
    std::uniform_int_distribution<int> side_dist(0, 1);
    std::uniform_int_distribution<int> action_dist(0, 9); // 10% cancel, 90% new order

    std::vector<uint64_t> latencies;
    latencies.reserve(NUM_OPERATIONS);

    std::cout << "Pre-generating order workload...\n";
    std::vector<quant::OrderRequest> requests;
    requests.reserve(NUM_OPERATIONS);

    for (size_t i = 1; i <= NUM_OPERATIONS; ++i) {
        quant::OrderRequest req;
        req.id = i;
        req.side = side_dist(rng) ? quant::Side::BUY : quant::Side::SELL;
        req.order_type = quant::OrderType::LIMIT;
        req.price = price_dist(rng);
        req.quantity = qty_dist(rng);
        req.timestamp = quant::currentTimestampNs();

        if (i > 1000 && action_dist(rng) == 0) {
            req.request_type = quant::RequestType::CANCEL;
            req.id = i - 500; // Cancel an earlier order
        } else {
            req.request_type = quant::RequestType::NEW;
        }
        requests.push_back(req);
    }

    std::cout << "Running benchmark on 1,000,000 operations...\n";
    auto start_time = std::chrono::high_resolution_clock::now();

    for (const auto& req : requests) {
        auto t0 = std::chrono::high_resolution_clock::now();
        engine.processRequest(req);
        auto t1 = std::chrono::high_resolution_clock::now();

        latencies.push_back(
            static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::nanoseconds>(t1 - t0).count())
        );
    }

    auto end_time = std::chrono::high_resolution_clock::now();
    double total_sec = std::chrono::duration<double>(end_time - start_time).count();

    auto stats = computeStats(latencies, total_sec);
    printStats("Direct Hot-Path Ingestion Benchmark", stats);

    std::cout << "Matching Engine Cumulative Summary:\n";
    std::cout << " - Total Orders Ingested : " << engine.totalOrdersProcessed() << "\n";
    std::cout << " - Total Trades Matched  : " << engine.totalTradesExecuted() << "\n";
    std::cout << " - Total Volume Executed : " << engine.totalVolumeTraded() << "\n";
    std::cout << " - Active Resting Orders : " << engine.orderBook().totalOrders() << "\n";

    return 0;
}
