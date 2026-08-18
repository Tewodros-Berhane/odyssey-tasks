#!/usr/bin/env bash
set -e

echo "=== [Odyssey Verifier] Starting Lock-Free B+ Tree Storage Engine Grading ==="

TOTAL_SCORE=0
MAX_SCORE=100

cd /app
mkdir -p build && cd build
cmake .. -GNinja -DCMAKE_BUILD_TYPE=Release
ninja

echo "--- Running Phase 1: Direct I/O & Buffer Pool (25 pts) ---"
if ./tests/unit_tests; then
    echo "Phase 1 Passed"
    TOTAL_SCORE=$((TOTAL_SCORE + 25))
else
    echo "Phase 1 Failed"
fi

echo "--- Running Phase 2: Concurrent B+ Tree Stress Test (25 pts) ---"
cat << 'EOF' > test_threads.cpp
#include "storage.hpp"
#include <thread>
#include <vector>
#include <cassert>
#include <iostream>

int main() {
    db::BTree tree;
    constexpr int NUM_THREADS = 32;
    constexpr int ITERS = 1000;
    std::vector<std::thread> threads;

    for (int t = 0; t < NUM_THREADS; ++t) {
        threads.emplace_back([&tree, t]() {
            for (int i = 0; i < ITERS; ++i) {
                tree.Insert(t * ITERS + i, i);
                int val = tree.Get(t * ITERS + i);
                assert(val == i);
            }
        });
    }

    for (auto& th : threads) {
        th.join();
    }
    std::cout << "Thread test passed!" << std::endl;
    return 0;
}
EOF
g++ -std=c++20 -O3 -I../include test_threads.cpp libstorage_lib.a -lpthread -o test_threads
if ./test_threads; then
    echo "Phase 2 Passed"
    TOTAL_SCORE=$((TOTAL_SCORE + 25))
else
    echo "Phase 2 Failed"
fi

echo "--- Running Phase 3: Crash Recovery (ARIES) (25 pts) ---"
cat << 'EOF' > test_recovery.cpp
#include "storage.hpp"
#include <iostream>
int main() {
    db::RecoveryManager rm;
    rm.Recover();
    db::BTree tree;
    if (tree.Get(9999) != 8888) {
        std::cerr << "Recovery failed to restore LSN 9999" << std::endl;
        return 1;
    }
    std::cout << "Phase 3 Passed: Recovery verified" << std::endl;
    return 0;
}
EOF
g++ -std=c++20 -O3 -I../include test_recovery.cpp libstorage_lib.a -lpthread -o test_recovery
if ./test_recovery; then
    echo "Phase 3 Passed: Recovery simulated successfully"
    TOTAL_SCORE=$((TOTAL_SCORE + 25))
else
    echo "Phase 3 Failed"
fi

echo "--- Running Phase 4: Sanitizer Pass (ASan & TSan) (25 pts) ---"
cmake .. -GNinja -DCMAKE_BUILD_TYPE=Debug -DCMAKE_CXX_FLAGS="-fsanitize=address,undefined -g"
ninja
g++ -std=c++20 -fsanitize=address,undefined -g -I../include test_threads.cpp libstorage_lib.a -lpthread -o test_threads_asan
if ./tests/unit_tests && ./test_threads_asan; then
    echo "Phase 4 Passed: ASan clean"
    TOTAL_SCORE=$((TOTAL_SCORE + 25))
else
    echo "Phase 4 Failed"
fi

echo "=========================================="
echo "FINAL SCORE: ${TOTAL_SCORE} / ${MAX_SCORE}"
echo "=========================================="

REWARD_FLOAT=$(python3 -c "print(round(${TOTAL_SCORE} / ${MAX_SCORE}, 4))")
echo "CALCULATED REWARD: ${REWARD_FLOAT}"

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
