#!/usr/bin/env bash
set -e

echo "=== [Odyssey Verifier] Starting MVCC B-Tree Engine Grading ==="

TOTAL_SCORE=0
MAX_SCORE=100

cd /app

mkdir -p build && cd build
cmake .. -GNinja -DCMAKE_BUILD_TYPE=Release
ninja

echo "--- Running Phase 1: Functional Tests (25 pts) ---"
if ./tests/unit_tests; then
    echo "Phase 1 Passed: Functional CRUD & Range Scans"
    TOTAL_SCORE=$((TOTAL_SCORE + 25))
else
    echo "Phase 1 Failed: Functional CRUD"
fi

echo "--- Running Phase 2: Concurrent Concurrency & Snapshot Isolation (25 pts) ---"
cat << 'EOF' > test_concurrent.cpp
#include "engine.hpp"
#include <thread>
#include <vector>
#include <cassert>
#include <iostream>
#include <atomic>

int main() {
    mvcc::StorageEngine engine("concurrency_test.db");
    assert(engine.Open());

    constexpr int NUM_THREADS = 16;
    constexpr int TXNS_PER_THREAD = 1000;
    std::vector<std::thread> workers;
    std::atomic<int> committed{0};

    for (int t = 0; t < NUM_THREADS; ++t) {
        workers.emplace_back([&, t]() {
            for (int i = 0; i < TXNS_PER_THREAD; ++i) {
                auto txn = engine.BeginTransaction();
                int64_t key = (i * 17) % 500;
                std::string val = "thread_" + std::to_string(t) + "_val_" + std::to_string(i);
                engine.Put(*txn, key, val);
                if (txn->Commit()) {
                    committed.fetch_add(1);
                }
            }
        });
    }

    for (auto& w : workers) w.join();
    std::cout << "Successfully executed concurrent transactions. Total committed: " << committed.load() << std::endl;
    assert(committed.load() > 0);
    engine.Close();
    return 0;
}
EOF
g++ -std=c++20 -O3 -I../include test_concurrent.cpp libmvcc_engine.a -lpthread -o test_concurrent
if ./test_concurrent; then
    echo "Phase 2 Passed: Concurrency & Snapshot Isolation"
    TOTAL_SCORE=$((TOTAL_SCORE + 25))
else
    echo "Phase 2 Failed: Concurrency"
fi

echo "--- Running Phase 3: ARIES Crash Recovery Simulation (30 pts) ---"
cat << 'EOF' > test_crash_recovery.cpp
#include "engine.hpp"
#include "wal.hpp"
#include <cassert>
#include <iostream>
#include <filesystem>

int main() {
    std::filesystem::remove("crash_test.db.wal");
    {
        mvcc::StorageEngine engine("crash_test.db");
        assert(engine.Open());
        
        auto txn1 = engine.BeginTransaction();
        engine.Put(*txn1, 10, "persisted_10");
        assert(txn1->Commit());

        auto txn2 = engine.BeginTransaction();
        engine.Put(*txn2, 20, "uncommitted_20");
        // Simulate abrupt SIGKILL / process crash without commit
    }

    // Recover database from disk WAL
    {
        mvcc::StorageEngine engine("crash_test.db");
        assert(engine.Open());
        assert(engine.Recover());

        auto read_txn = engine.BeginTransaction();
        auto val10 = engine.Get(*read_txn, 10);
        assert(val10.has_value() && *val10 == "persisted_10");

        auto val20 = engine.Get(*read_txn, 20);
        assert(!val20.has_value()); // Uncommitted transaction must be rolled back by Undo phase
        read_txn->Commit();
    }
    std::cout << "ARIES Crash Recovery successfully passed!" << std::endl;
    return 0;
}
EOF
g++ -std=c++20 -O3 -I../include test_crash_recovery.cpp libmvcc_engine.a -lpthread -o test_crash_recovery
if ./test_crash_recovery; then
    echo "Phase 3 Passed: ARIES Crash Recovery"
    TOTAL_SCORE=$((TOTAL_SCORE + 30))
else
    echo "Phase 3 Failed: ARIES Crash Recovery"
fi

echo "--- Running Phase 4: ThreadSanitizer & AddressSanitizer Validation (20 pts) ---"
cmake .. -GNinja -DCMAKE_BUILD_TYPE=Debug -DCMAKE_CXX_FLAGS="-fsanitize=address,undefined -g"
ninja
if ./tests/unit_tests; then
    echo "Phase 4 Passed: Clean ASan/UBSan report"
    TOTAL_SCORE=$((TOTAL_SCORE + 20))
else
    echo "Phase 4 Failed: Sanitizer violations detected"
fi

echo "=========================================="
echo "FINAL SCORE: ${TOTAL_SCORE} / ${MAX_SCORE}"
echo "=========================================="

if [ "${TOTAL_SCORE}" -ge 80 ]; then
    echo "VERDICT: SUCCESS"
    exit 0
else
    echo "VERDICT: FAILURE (Score below 80%)"
    exit 1
fi
