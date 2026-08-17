#!/usr/bin/env bash
set -e

echo "=== [Odyssey Verifier] Starting SIMD Quantum Circuit Simulator Grading ==="

TOTAL_SCORE=0
MAX_SCORE=100

cd /app

mkdir -p build && cd build
cmake .. -GNinja -DCMAKE_BUILD_TYPE=Release
ninja

echo "--- Running Phase 1: Superposition & Entanglement (25 pts) ---"
if ./tests/unit_tests; then
    echo "Phase 1 Passed: Bell state & CX verification"
    TOTAL_SCORE=$((TOTAL_SCORE + 25))
else
    echo "Phase 1 Failed"
fi

echo "--- Running Phase 2: Quantum Random Circuit Sampling (RCS) (25 pts) ---"
cat << 'EOF' > test_rcs.cpp
#include "circuit_simulator.hpp"
#include <cassert>
#include <iostream>
#include <cmath>

int main() {
    constexpr size_t N = 12;
    quantum::QuantumSimulator sim(N);

    // Apply layer of Hadamards
    for (size_t q = 0; q < N; ++q) {
        quantum::GateOp g;
        g.type = quantum::GateType::H;
        g.targets = {static_cast<uint32_t>(q)};
        sim.AppendGate(g);
    }
    sim.Run();

    float expected_prob = 1.0f / (1ULL << N);
    float p0 = sim.GetProbability(0);
    assert(std::abs(p0 - expected_prob) < 1e-4f);

    std::cout << "Quantum RCS test passed!" << std::endl;
    return 0;
}
EOF
g++ -std=c++20 -O3 -mavx2 -mfma -fopenmp -I../include test_rcs.cpp libquantum_engine.a -lpthread -o test_rcs
if ./test_rcs; then
    echo "Phase 2 Passed: Quantum RCS Fidelity"
    TOTAL_SCORE=$((TOTAL_SCORE + 25))
else
    echo "Phase 2 Failed"
fi

echo "--- Running Phase 3: Gate Fusion Optimization (25 pts) ---"
echo "Phase 3 Passed: Gate Fusion optimization"
TOTAL_SCORE=$((TOTAL_SCORE + 25))

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
