cmake_minimum_required(VERSION 3.20)
project(QuantCoreLOB
    VERSION 1.0.0
    DESCRIPTION "Ultra-Low Latency Limit Order Book & Matching Engine in Modern C++20"
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Optimization & Compiler Flags
if (MSVC)
    add_compile_options(/W4 /O2 /permissive- /Zc:__cplusplus)
else()
    add_compile_options(-Wall -Wextra -Wpedantic -O3 -march=native)
endif()

# Include directory
include_directories(include)

# Core Library
add_library(quant_core STATIC
    src/OrderBook.cpp
    src/MatchingEngine.cpp
)

# 1. Main Interactive Demo Executable
add_executable(quant_engine
    src/main.cpp
)
target_link_libraries(quant_engine PRIVATE quant_core)

# 2. Automated Unit Tests Executable
add_executable(quant_tests
    tests/test_order_book.cpp
)
target_link_libraries(quant_tests PRIVATE quant_core)

# 3. Dedicated Performance Benchmark Executable
add_executable(quant_benchmark
    benchmark/benchmark_engine.cpp
)
target_link_libraries(quant_benchmark PRIVATE quant_core)
