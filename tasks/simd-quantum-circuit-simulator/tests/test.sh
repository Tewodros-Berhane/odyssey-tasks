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
    if (std::abs(p0 - expected_prob) > 1e-4f) {
        std::cerr << "RCS validation failed: expected " << expected_prob << ", got " << p0 << std::endl;
        return 1;
    }

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
cat << 'EOF' > test_fusion.cpp
#include "circuit_simulator.hpp"
#include <cassert>
#include <iostream>
#include <cmath>

int main() {
    quantum::QuantumSimulator sim(3);
    quantum::GateOp g1; g1.type = quantum::GateType::H; g1.targets = {0};
    quantum::GateOp g2; g2.type = quantum::GateType::X; g2.targets = {1};
    quantum::GateOp g3; g3.type = quantum::GateType::CX; g3.targets = {2}; g3.controls = {0};
    sim.AppendGate(g1);
    sim.AppendGate(g2);
    sim.AppendGate(g3);
    sim.OptimizeDAG();
    sim.Run();

    float p0 = sim.GetProbability(0);
    // After H(0), X(1), CX(0->2): state is 1/sqrt(2) |010> + 1/sqrt(2) |111>
    // indices: |010> = 2, |111> = 7
    float p2 = sim.GetProbability(2);
    float p7 = sim.GetProbability(7);

    if (std::abs(p2 - 0.5f) > 1e-4f || std::abs(p7 - 0.5f) > 1e-4f) {
        std::cerr << "Gate fusion test failed: p2=" << p2 << ", p7=" << p7 << std::endl;
        return 1;
    }
    std::cout << "Gate fusion test passed!" << std::endl;
    return 0;
}
EOF
g++ -std=c++20 -O3 -mavx2 -mfma -fopenmp -I../include test_fusion.cpp libquantum_engine.a -lpthread -o test_fusion
if ./test_fusion; then
    echo "Phase 3 Passed: Gate Fusion optimization"
    TOTAL_SCORE=$((TOTAL_SCORE + 25))
else
    echo "Phase 3 Failed"
fi

echo "--- Running Phase 4: Sanitizer Pass (25 pts) ---"
cmake .. -GNinja -DCMAKE_BUILD_TYPE=Debug -DCMAKE_CXX_FLAGS="-fsanitize=address,undefined -g"
ninja
if ./tests/unit_tests && ./test_rcs; then
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
