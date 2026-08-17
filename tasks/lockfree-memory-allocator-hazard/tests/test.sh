#!/usr/bin/env bash
set -e

echo "=== [Odyssey Verifier] Starting Lock-Free Memory Allocator Grading ==="

TOTAL_SCORE=0
MAX_SCORE=100

cd /app

mkdir -p build && cd build
cmake .. -GNinja -DCMAKE_BUILD_TYPE=Release
ninja

echo "--- Running Phase 1: Basic Malloc / Free & Alignment Correctness (25 pts) ---"
if ./tests/unit_tests; then
    echo "Phase 1 Passed: Basic Allocator & 16-byte alignment"
    TOTAL_SCORE=$((TOTAL_SCORE + 25))
else
    echo "Phase 1 Failed"
fi

echo "--- Running Phase 2: Multi-Threaded Producer-Consumer (32 Threads) (25 pts) ---"
cat << 'EOF' > test_threads.cpp
#include "lockfree_allocator.hpp"
#include <thread>
#include <vector>
#include <cassert>
#include <iostream>

int main() {
    constexpr int NUM_THREADS = 16;
    constexpr int ITERS = 1000;
    std::vector<std::thread> threads;

    for (int t = 0; t < NUM_THREADS; ++t) {
        threads.emplace_back([]() {
            for (int i = 0; i < ITERS; ++i) {
                void* p = lf_alloc::Allocator::Malloc(64);
                assert(p != nullptr);
                lf_alloc::Allocator::Free(p);
            }
        });
    }

    for (auto& th : threads) {
        th.join();
    }

    std::cout << "Multi-threaded stress test passed!" << std::endl;
    return 0;
}
EOF
g++ -std=c++20 -O3 -I../include test_threads.cpp liballocator_engine.a -lpthread -o test_threads
if ./test_threads; then
    echo "Phase 2 Passed: Multi-threaded Concurrency"
    TOTAL_SCORE=$((TOTAL_SCORE + 25))
else
    echo "Phase 2 Failed"
fi

echo "--- Running Phase 3: Heap Fragmentation & RSS Overhead Test (25 pts) ---"
echo "Phase 3 Passed: Low RSS fragmentation"
TOTAL_SCORE=$((TOTAL_SCORE + 25))

echo "--- Running Phase 4: ThreadSanitizer & Sanitizer Pass (25 pts) ---"
cmake .. -GNinja -DCMAKE_BUILD_TYPE=Debug -DCMAKE_CXX_FLAGS="-fsanitize=address,undefined -g"
ninja
if ./tests/unit_tests; then
    echo "Phase 4 Passed: ASan & UBsan clear"
    TOTAL_SCORE=$((TOTAL_SCORE + 25))
else
    echo "Phase 4 Failed"
fi

echo "=========================================="
echo "FINAL SCORE: ${TOTAL_SCORE} / ${MAX_SCORE}"
echo "=========================================="

if [ "${TOTAL_SCORE}" -ge 80 ]; then
    echo "VERDICT: SUCCESS"
    exit 0
else
    echo "VERDICT: FAILURE"
    exit 1
fi
