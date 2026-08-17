#!/usr/bin/env bash
set -e

echo "=== [Odyssey Verifier] Starting HNSW PQ SIMD Vector Engine Grading ==="

TOTAL_SCORE=0
MAX_SCORE=100

cd /app

mkdir -p build && cd build
cmake .. -GNinja -DCMAKE_BUILD_TYPE=Release
ninja

echo "--- Running Phase 1: Unit & SIMD Correctness (25 pts) ---"
if ./tests/unit_tests; then
    echo "Phase 1 Passed: SIMD & basic HNSW"
    TOTAL_SCORE=$((TOTAL_SCORE + 25))
else
    echo "Phase 1 Failed"
fi

echo "--- Running Phase 2: Concurrent Multi-Threaded Insertion (25 pts) ---"
cat << 'EOF' > test_concurrent_insert.cpp
#include "hnsw.hpp"
#include <thread>
#include <vector>
#include <cassert>
#include <iostream>

int main() {
    vecengine::HNSWIndex index(768, 16, 64, 32);
    constexpr int NUM_THREADS = 8;
    constexpr int VECS_PER_THREAD = 100;
    std::vector<std::thread> threads;

    for (int t = 0; t < NUM_THREADS; ++t) {
        threads.emplace_back([&, t]() {
            std::vector<float> vec(768, static_cast<float>(t));
            for (int i = 0; i < VECS_PER_THREAD; ++i) {
                vec[0] = static_cast<float>(i);
                index.Insert(t * VECS_PER_THREAD + i, vec);
            }
        });
    }

    for (auto& th : threads) th.join();
    assert(index.Size() == NUM_THREADS * VECS_PER_THREAD);
    std::cout << "Concurrent insertion passed! Total items: " << index.Size() << std::endl;
    return 0;
}
EOF
g++ -std=c++20 -O3 -mavx2 -mfma -I../include test_concurrent_insert.cpp libhnsw_engine.a -lpthread -o test_concurrent_insert
if ./test_concurrent_insert; then
    echo "Phase 2 Passed: Multi-threaded insertion"
    TOTAL_SCORE=$((TOTAL_SCORE + 25))
else
    echo "Phase 2 Failed"
fi

echo "--- Running Phase 3: Recall & QPS Throughput Benchmark (30 pts) ---"
cat << 'EOF' > test_benchmark.cpp
#include "hnsw.hpp"
#include <chrono>
#include <cassert>
#include <iostream>

int main() {
    constexpr size_t NUM_VECS = 1000;
    constexpr size_t NUM_QUERIES = 200;
    vecengine::HNSWIndex index(768, 32, 128, 64);

    std::vector<float> base_data(NUM_VECS * 768, 0.5f);
    for (size_t i = 0; i < NUM_VECS; ++i) {
        base_data[i * 768 + (i % 768)] = 1.5f;
        index.Insert(i, std::span<const float>(&base_data[i * 768], 768));
    }

    auto start = std::chrono::high_resolution_clock::now();
    size_t hits = 0;
    for (size_t q = 0; q < NUM_QUERIES; ++q) {
        auto query = std::span<const float>(&base_data[q * 768], 768);
        auto results = index.SearchKNN(query, 10);
        for (const auto& r : results) {
            if (r.id == q) {
                hits++;
                break;
            }
        }
    }
    auto end = std::chrono::high_resolution_clock::now();
    double duration_sec = std::chrono::duration<double>(end - start).count();
    double qps = NUM_QUERIES / duration_sec;
    double recall = static_cast<double>(hits) / NUM_QUERIES;

    std::cout << "Recall@10: " << recall << ", QPS: " << qps << std::endl;
    assert(recall >= 0.80);
    return 0;
}
EOF
g++ -std=c++20 -O3 -mavx2 -mfma -I../include test_benchmark.cpp libhnsw_engine.a -lpthread -o test_benchmark
if ./test_benchmark; then
    echo "Phase 3 Passed: Recall and Throughput benchmark"
    TOTAL_SCORE=$((TOTAL_SCORE + 30))
else
    echo "Phase 3 Failed"
fi

echo "--- Running Phase 4: Sanitizer Pass (20 pts) ---"
cmake .. -GNinja -DCMAKE_BUILD_TYPE=Debug -DCMAKE_CXX_FLAGS="-fsanitize=address,undefined -g"
ninja
if ./tests/unit_tests; then
    echo "Phase 4 Passed: ASan & UBsan clear"
    TOTAL_SCORE=$((TOTAL_SCORE + 20))
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
