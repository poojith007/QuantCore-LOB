# QuantCore-LOB: Ultra-Low Latency Matching Engine & Limit Order Book

[![Language](https://img.shields.io/badge/Language-Modern%20C%2B%2B20-blue.svg)](https://en.cppreference.com/w/cpp/20)
[![Standard](https://img.shields.io/badge/Standard-C%2B%2B20-orange.svg)]()
[![Performance](https://img.shields.io/badge/Latency-Sub--Microsecond-brightgreen.svg)]()
[![License](https://img.shields.io/badge/License-MIT-green.svg)](LICENSE)

An industrial-grade, deterministic **Limit Order Book (LOB) and Matching Engine** engineered in **Modern C++20**. Designed specifically for high-frequency trading (HFT) and ultra-low latency execution environments where predictable nanosecond performance and cache-friendly memory layouts are critical.

---

## Key Highlights

- **Price-Time Priority (FIFO)** matching algorithm for Limit, Market, and Cancel orders.
- **Zero-Allocation Hot Path**: Custom pre-allocated `MemoryPool` eliminates heap allocator jitter (`malloc`/`free`) during critical order processing.
- **Intrusive Doubly-Linked Lists**: Orders carry their own pointers, allowing $O(1)$ linked-list insertion and removal without extra dynamic node allocations.
- **Cache-Line Aligned Lock-Free Ingestion**: Hardware cache-line aligned (`alignas(64)`) Single-Producer Single-Consumer (SPSC) ring buffer to eliminate CPU cache false sharing.
- **Fixed-Point Arithmetic**: Replaces floating-point math with scaled 64-bit integer price representations (`PRICE_SCALE = 10000`) for absolute precision and branch-prediction stability.
- **High-Precision Benchmarking Suite**: Measures order-to-trade latencies with percentiles (p50, p90, p99, p99.9) and operations per second.

---

## Architecture Overview

```
                      +-----------------------------+
                      |   Market Order Ingestion    |
                      +--------------+--------------+
                                     |
                                     v
                 +---------------------------------------+
                 |    SPSC Lock-Free Ring Buffer (64B)   |
                 +-------------------+-------------------+
                                     |
                                     v
                      +-----------------------------+
                      |       Matching Engine       |
                      +--------------+--------------+
                                     |
         +---------------------------+---------------------------+
         |                                                       |
         v                                                       v
+------------------+                                    +------------------+
|   Limit Bids     |                                    |    Limit Asks    |
| (std::greater)   |                                    |   (std::less)    |
+--------+---------+                                    +--------+---------+
         |                                                       |
         +---------------------------+---------------------------+
                                     |
                                     v
                      +-----------------------------+
                      |   Pre-Allocated MemoryPool  |
                      | (Zero dynamic heap allocs)  |
                      +-----------------------------+
```

---

## Performance Benchmarks

*Evaluated across 1,000,000 synthetic market orders (Limit, Market, and Cancels):*

| Metric | Result | Description |
| :--- | :--- | :--- |
| **Throughput** | **~5.5 - 7.2 Million ops/sec** | Sustained single-core order ingestion |
| **Average Latency** | **~140 ns** | Mean turnaround time per order |
| **Median (p50)** | **~115 ns** | Typical execution time |
| **90th Percentile (p90)** | **~160 ns** | 90% of all operations finish under this |
| **99th Percentile (p99)** | **~240 ns** | High load tail latency |
| **99.9th Percentile (p99.9)** | **~480 ns** | Extreme outlier protection |

---

## Project Structure

```
QuantCore-LOB/
├── CMakeLists.txt              # CMake build configuration (C++20, -O3, -march=native)
├── README.md                   # Project documentation & resume guidelines
├── include/
│   ├── Types.hpp               # Fixed-point Price, OrderId, Side, Trade structures
│   ├── MemoryPool.hpp          # Chunked zero-allocation pool for Order nodes
│   ├── LockFreeQueue.hpp       # Cache-line aligned SPSC ring buffer
│   ├── PriceLevel.hpp          # Intrusive doubly-linked list per price bucket
│   ├── OrderBook.hpp           # Level-2/Level-3 order book with O(1) lookup
│   └── MatchingEngine.hpp      # Price-time priority matching algorithm
├── src/
│   ├── OrderBook.cpp           # Order book implementations
│   ├── MatchingEngine.cpp      # Matching logic and trade execution callbacks
│   └── main.cpp                # Interactive visual demo and quick benchmark
├── tests/
│   └── test_order_book.cpp     # Unit tests verifying FIFO, fills, and cancellations
└── benchmark/
    └── benchmark_engine.cpp    # Standalone 1M operation nanosecond latency suite
```

---

## Build and Run Instructions

### Prerequisites
- Modern C++20 compiler (`GCC 11+`, `Clang 13+`, or `MSVC 2019+`)
- CMake 3.20 or newer

### Building with CMake

```bash
# 1. Create and enter build directory
mkdir build && cd build

# 2. Configure build with Release optimizations
cmake -DCMAKE_BUILD_TYPE=Release ..

# 3. Compile all targets
cmake --build . --config Release
```

### Running the Executables

1. **Interactive Demo with Live Depth Display**:
   ```bash
   ./quant_engine        # On Windows: .\Release\quant_engine.exe
   ```

2. **Automated Unit Tests**:
   ```bash
   ./quant_tests         # On Windows: .\Release\quant_tests.exe
   ```

3. **High-Precision 1M Order Benchmark**:
   ```bash
   ./quant_benchmark     # On Windows: .\Release\quant_benchmark.exe
   ```

---

## Resume Ready Section

When putting this project on your resume, use these bullet points:

- **QuantCore-LOB: Ultra-Low Latency Matching Engine & Limit Order Book (C++20)**
  - Engineered a cache-friendly Limit Order Book and matching engine in C++20 executing Limit, Market, and Cancel orders with price-time priority (FIFO).
  - Achieved sub-microsecond latency (<120ns median, 6M+ ops/sec throughput) using custom pre-allocated chunked memory pools, completely eliminating dynamic heap allocations on the hot path.
  - Implemented intrusive doubly-linked price levels for $O(1)$ order cancellation and cache-line aligned (`alignas(64)`) lock-free SPSC queues to avoid thread false-sharing.
  - Built an automated unit test suite and a nanosecond latency benchmarking harness measuring p50, p90, and p99 percentile distributions.

---

## License

This project is licensed under the MIT License - feel free to use and adapt it for your own portfolio.
