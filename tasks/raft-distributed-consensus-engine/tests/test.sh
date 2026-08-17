#!/usr/bin/env bash
set -e

echo "=== [Odyssey Verifier] Starting Raft Distributed Consensus Grading ==="

TOTAL_SCORE=0
MAX_SCORE=100

cd /app

mkdir -p build && cd build
cmake .. -GNinja -DCMAKE_BUILD_TYPE=Release
ninja

echo "--- Running Phase 1: Basic 3-Node Election & Logging (25 pts) ---"
if ./tests/unit_tests; then
    echo "Phase 1 Passed: Basic Election & Ticks"
    TOTAL_SCORE=$((TOTAL_SCORE + 25))
else
    echo "Phase 1 Failed"
fi

echo "--- Running Phase 2: Network Partition & Pre-Vote Isolation (25 pts) ---"
cat << 'EOF' > test_chaos_partition.cpp
#include "raft_cluster.hpp"
#include <cassert>
#include <iostream>

int main() {
    raft::RaftCluster cluster;
    auto s1 = std::make_shared<raft::MemoryStorage>();
    auto s2 = std::make_shared<raft::MemoryStorage>();
    auto s3 = std::make_shared<raft::MemoryStorage>();
    auto s4 = std::make_shared<raft::MemoryStorage>();
    auto s5 = std::make_shared<raft::MemoryStorage>();

    auto n1 = std::make_shared<raft::RaftNode>(1, std::vector<raft::node_id_t>{2, 3, 4, 5}, s1, &cluster);
    auto n2 = std::make_shared<raft::RaftNode>(2, std::vector<raft::node_id_t>{1, 3, 4, 5}, s2, &cluster);
    auto n3 = std::make_shared<raft::RaftNode>(3, std::vector<raft::node_id_t>{1, 2, 4, 5}, s3, &cluster);
    auto n4 = std::make_shared<raft::RaftNode>(4, std::vector<raft::node_id_t>{1, 2, 3, 5}, s4, &cluster);
    auto n5 = std::make_shared<raft::RaftNode>(5, std::vector<raft::node_id_t>{1, 2, 3, 4}, s5, &cluster);

    cluster.AddNode(n1);
    cluster.AddNode(n2);
    cluster.AddNode(n3);
    cluster.AddNode(n4);
    cluster.AddNode(n5);

    // Initial stabilization
    for (int i = 0; i < 20; ++i) cluster.TickAll();

    // Partition cluster: {1, 2, 3} vs {4, 5}
    cluster.SetPartition({1, 2, 3}, {4, 5});
    for (int i = 0; i < 30; ++i) cluster.TickAll();

    // Heal partition
    cluster.ClearPartitions();
    for (int i = 0; i < 30; ++i) cluster.TickAll();

    std::cout << "Chaos partition test successfully completed!" << std::endl;
    return 0;
}
EOF
g++ -std=c++20 -O3 -I../include test_chaos_partition.cpp libraft_engine.a -lpthread -o test_chaos_partition
if ./test_chaos_partition; then
    echo "Phase 2 Passed: Partition & Pre-vote recovery"
    TOTAL_SCORE=$((TOTAL_SCORE + 25))
else
    echo "Phase 2 Failed"
fi

echo "--- Running Phase 3: Snapshot Compaction & Replay (25 pts) ---"
cat << 'EOF' > test_snapshot.cpp
#include "raft_storage.hpp"
#include <cassert>
#include <iostream>

int main() {
    raft::MemoryStorage storage;
    std::vector<raft::LogEntry> entries;
    for (uint64_t i = 1; i <= 100; ++i) {
        raft::LogEntry e;
        e.index = i;
        e.term = 1;
        e.data = "entry_" + std::to_string(i);
        entries.push_back(e);
    }
    storage.Append(entries);
    assert(storage.GetLastIndex() == 100);

    // Compact up to index 80
    storage.ApplySnapshot(80, 1, "snapshot_blob_at_80");
    assert(storage.GetLastIndex() == 80);
    assert(storage.GetTerm(80) == 1);
    assert(storage.GetTerm(50) == 0); // Truncated

    std::cout << "Snapshot compaction passed!" << std::endl;
    return 0;
}
EOF
g++ -std=c++20 -O3 -I../include test_snapshot.cpp libraft_engine.a -lpthread -o test_snapshot
if ./test_snapshot; then
    echo "Phase 3 Passed: Snapshot Compaction"
    TOTAL_SCORE=$((TOTAL_SCORE + 25))
else
    echo "Phase 3 Failed"
fi

echo "--- Running Phase 4: Sanitizer Pass (25 pts) ---"
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
