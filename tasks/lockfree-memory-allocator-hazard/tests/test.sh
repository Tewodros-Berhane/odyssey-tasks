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

echo "--- Running Phase 2: Multi-Threaded Producer-Consumer (16 Threads) (25 pts) ---"
cat << 'EOF' > test_threads.cpp
#include "lockfree_allocator.hpp"
#include <thread>
#include <vector>
#include <cassert>
#include <iostream>

int main() {
    constexpr int NUM_THREADS = 16;
    constexpr int ITERS = 500;
    std::vector<std::thread> threads;

    for (int t = 0; t < NUM_THREADS; ++t) {
        threads.emplace_back([]() {
            for (int i = 0; i < ITERS; ++i) {
                void* p = lf_alloc::Allocator::Malloc(64);
                if (!p) {
                    std::cerr << "Allocation failed" << std::endl;
                    std::exit(1);
                }
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

echo "--- Running Phase 3: Heap Fragmentation & Allocation Realloc Test (25 pts) ---"
cat << 'EOF' > test_realloc.cpp
#include "lockfree_allocator.hpp"
#include <cassert>
#include <iostream>
#include <cstring>

int main() {
    void* p = lf_alloc::Allocator::Malloc(32);
    if (!p) return 1;
    std::memset(p, 0xAB, 32);

    void* p_new = lf_alloc::Allocator::Realloc(p, 128);
    if (!p_new) return 1;

    unsigned char* b = static_cast<unsigned char*>(p_new);
    for (int i = 0; i < 32; ++i) {
        if (b[i] != 0xAB) {
            std::cerr << "Realloc data preservation failed" << std::endl;
            return 1;
        }
    }
    lf_alloc::Allocator::Free(p_new);
    std::cout << "Realloc test passed!" << std::endl;
    return 0;
}
EOF
g++ -std=c++20 -O3 -I../include test_realloc.cpp liballocator_engine.a -lpthread -o test_realloc
if ./test_realloc; then
    echo "Phase 3 Passed: Realloc & Fragmentation"
    TOTAL_SCORE=$((TOTAL_SCORE + 25))
else
    echo "Phase 3 Failed"
fi

echo "--- Running Phase 4: AddressSanitizer & UndefinedSanitizer Pass (25 pts) ---"
cmake .. -GNinja -DCMAKE_BUILD_TYPE=Debug -DCMAKE_CXX_FLAGS="-fsanitize=address,undefined -g"
ninja
g++ -std=c++20 -fsanitize=address,undefined -g -I../include test_threads.cpp liballocator_engine.a -lpthread -o test_threads_asan
if ./tests/unit_tests && ./test_threads_asan; then
    echo "Phase 4 Passed: ASan & UBsan clear"
    TOTAL_SCORE=$((TOTAL_SCORE + 25))
else
    echo "Phase 4 Failed"
fi

echo "=========================================="
echo "FINAL SCORE: ${TOTAL_SCORE} / ${MAX_SCORE}"
echo "=========================================="

# Calculate reward scalar
REWARD_FLOAT=$(python3 -c "print(round(${TOTAL_SCORE} / ${MAX_SCORE}, 4))")
echo "CALCULATED REWARD: ${REWARD_FLOAT}"

# Write reward files across all candidate locations
for d in verifier /tmp/verifier /logs/verifier /app .; do
    mkdir -p "$d" 2>/dev/null || true
done

targets=(
    "verifier/reward.txt"
    "reward.txt"
    "/tmp/verifier/reward.txt"
    "/tmp/reward.txt"
    "/logs/verifier/reward.txt"
    "/app/reward.txt"
)

for t in "${targets[@]}"; do
    echo "$REWARD_FLOAT" > "$t" 2>/dev/null || true
done

json_targets=(
    "reward.json"
    "verifier/reward.json"
    "/tmp/verifier/reward.json"
    "/tmp/reward.json"
    "/logs/verifier/reward.json"
    "/app/reward.json"
)

for jt in "${json_targets[@]}"; do
    echo "{\"reward\": $REWARD_FLOAT}" > "$jt" 2>/dev/null || true
done

if [ "${TOTAL_SCORE}" -ge 80 ]; then
    echo "VERDICT: SUCCESS"
    exit 0
else
    echo "VERDICT: FAILURE"
    exit 1
fi
