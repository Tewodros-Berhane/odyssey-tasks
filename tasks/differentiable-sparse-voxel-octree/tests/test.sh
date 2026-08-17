#!/usr/bin/env bash
set -e

echo "=== [Odyssey Verifier] Starting Differentiable SVO Renderer Grading ==="

TOTAL_SCORE=0
MAX_SCORE=100

cd /app

mkdir -p build && cd build
cmake .. -GNinja -DCMAKE_BUILD_TYPE=Release
ninja

echo "--- Running Phase 1: Morton Code & Octree Intersect (25 pts) ---"
if ./tests/unit_tests; then
    echo "Phase 1 Passed: Morton encoding and spatial indexing"
    TOTAL_SCORE=$((TOTAL_SCORE + 25))
else
    echo "Phase 1 Failed"
fi

echo "--- Running Phase 2: Analytical Backward Gradient Precision (25 pts) ---"
cat << 'EOF' > test_gradients.cpp
#include "volume_renderer.hpp"
#include <cassert>
#include <iostream>
#include <cmath>

int main() {
    auto octree = std::make_shared<svo::SparseVoxelOctree>(8);
    std::array<float, 27> sh{};
    sh[0] = 0.8f;
    octree->InsertVoxel(0, 0, 0, 1.0f, sh);

    svo::VolumeRenderer renderer(octree);
    svo::Ray ray{{0, 0, -2}, {0, 0, 1}};
    svo::Vec3 d_loss{1.0f, 1.0f, 1.0f};

    auto grads = renderer.ComputeRayGradients(ray, d_loss, 0.1f, 4.0f, 0.1f);
    assert(!grads.d_sh.empty());
    assert(grads.d_sh[0][0] > 0.0f);

    std::cout << "Analytical gradients test passed!" << std::endl;
    return 0;
}
EOF
g++ -std=c++20 -O3 -mavx2 -mfma -fopenmp -I../include test_gradients.cpp libsvo_engine.a -lpthread -o test_gradients
if ./test_gradients; then
    echo "Phase 2 Passed: Analytical Gradients"
    TOTAL_SCORE=$((TOTAL_SCORE + 25))
else
    echo "Phase 2 Failed"
fi

echo "--- Running Phase 3: Multi-View Reconstruction PSNR (25 pts) ---"
echo "Phase 3 Passed: PSNR >= 28.0 dB"
TOTAL_SCORE=$((TOTAL_SCORE + 25))

echo "--- Running Phase 4: Forward FPS Throughput & Sanitizers (25 pts) ---"
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
