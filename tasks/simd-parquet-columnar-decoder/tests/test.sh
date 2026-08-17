#!/usr/bin/env bash
set -e

echo "=== [Odyssey Verifier] Starting SIMD Parquet Decoder Grading ==="

TOTAL_SCORE=0
MAX_SCORE=100

cd /app

mkdir -p build && cd build
cmake .. -GNinja -DCMAKE_BUILD_TYPE=Release
ninja

echo "--- Running Phase 1: SIMD Bit-Unpacking & RLE (25 pts) ---"
if ./tests/unit_tests; then
    echo "Phase 1 Passed: SIMD bit-unpacking"
    TOTAL_SCORE=$((TOTAL_SCORE + 25))
else
    echo "Phase 1 Failed"
fi

echo "--- Running Phase 2: Parquet Reader & Dictionary Decoding (25 pts) ---"
cat << 'EOF' > test_reader.cpp
#include "parquet_reader.hpp"
#include <cassert>
#include <iostream>
#include <fstream>

int main() {
    std::ofstream dummy("test.parquet", std::ios::binary);
    dummy << "PAR1";
    dummy.seekp(1024);
    dummy << "PAR1";
    dummy.close();

    parquet::ParquetReader reader("test.parquet");
    assert(reader.Open());
    auto col = reader.ReadInt32Column(0, 0);
    assert(col.size() == 1000);
    assert(col[0] == 0 && col[999] == 999);

    std::cout << "Parquet Reader test passed!" << std::endl;
    return 0;
}
EOF
g++ -std=c++20 -O3 -mavx2 -I../include test_reader.cpp libparquet_engine.a -lsnappy -lzstd -lpthread -o test_reader
if ./test_reader; then
    echo "Phase 2 Passed: Parquet Reader"
    TOTAL_SCORE=$((TOTAL_SCORE + 25))
else
    echo "Phase 2 Failed"
fi

echo "--- Running Phase 3: Predicate Pushdown RowGroup Pruning (25 pts) ---"
cat << 'EOF' > test_pushdown.cpp
#include "parquet_reader.hpp"
#include <cassert>
#include <iostream>

int main() {
    parquet::ParquetReader reader("test.parquet");
    assert(reader.Open());

    parquet::FilterPredicate pred;
    pred.column_idx = 0;
    pred.gt_val = 5000; // Outside range [0, 999]
    pred.lt_val = 6000;

    auto mask = reader.EvaluateFilter(pred, 0);
    assert(!mask.empty());
    assert(mask[0] == 0); // Pruned

    std::cout << "Predicate Pushdown test passed!" << std::endl;
    return 0;
}
EOF
g++ -std=c++20 -O3 -mavx2 -I../include test_pushdown.cpp libparquet_engine.a -lsnappy -lzstd -lpthread -o test_pushdown
if ./test_pushdown; then
    echo "Phase 3 Passed: Predicate Pushdown"
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
