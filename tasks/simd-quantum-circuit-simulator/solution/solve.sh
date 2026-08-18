#!/usr/bin/env bash
set -e

echo "=== [Odyssey Oracle] Applying Quantum Simulator Reference Solution ==="

cat << 'EOF' > /app/src/simd_complex.cpp
#include "simd_complex.hpp"
#include <omp.h>

namespace quantum {

void SIMDComplex::ApplyGate1Q_AVX2(
    complex_t* state,
    size_t num_amplitudes,
    qubit_t target,
    const GateMatrix2x2& mat
) {
    size_t stride = 1ULL << target;
    size_t half_stride = stride;
    size_t num_blocks = num_amplitudes / (2 * stride);

    #pragma omp parallel for schedule(static)
    for (size_t b = 0; b < num_blocks; ++b) {
        size_t block_start = b * 2 * stride;
        for (size_t i = 0; i < half_stride; ++i) {
            size_t idx0 = block_start + i;
            size_t idx1 = idx0 + stride;

            complex_t a = state[idx0];
            complex_t b_val = state[idx1];

            state[idx0] = mat.m00 * a + mat.m01 * b_val;
            state[idx1] = mat.m10 * a + mat.m11 * b_val;
        }
    }
}

void SIMDComplex::ApplyGate2Q_AVX2(
    complex_t* state,
    size_t num_amplitudes,
    qubit_t q0,
    qubit_t q1,
    const GateMatrix4x4& mat
) {
    size_t s0 = 1ULL << q0;
    size_t s1 = 1ULL << q1;

    #pragma omp parallel for schedule(static)
    for (size_t i = 0; i < num_amplitudes; ++i) {
        if ((i & s0) == 0 && (i & s1) == 0) {
            size_t i00 = i;
            size_t i01 = i | s0;
            size_t i10 = i | s1;
            size_t i11 = i | s0 | s1;

            complex_t v0 = state[i00];
            complex_t v1 = state[i01];
            complex_t v2 = state[i10];
            complex_t v3 = state[i11];

            state[i00] = mat.data[0][0]*v0 + mat.data[0][1]*v1 + mat.data[0][2]*v2 + mat.data[0][3]*v3;
            state[i01] = mat.data[1][0]*v0 + mat.data[1][1]*v1 + mat.data[1][2]*v2 + mat.data[1][3]*v3;
            state[i10] = mat.data[2][0]*v0 + mat.data[2][1]*v1 + mat.data[2][2]*v2 + mat.data[2][3]*v3;
            state[i11] = mat.data[3][0]*v0 + mat.data[3][1]*v1 + mat.data[3][2]*v2 + mat.data[3][3]*v3;
        }
    }
}

} // namespace quantum
EOF

cat << 'EOF' > /app/src/circuit_simulator.cpp
#include "circuit_simulator.hpp"
#include "simd_complex.hpp"
#include <cmath>
#include <random>

namespace quantum {

QuantumSimulator::QuantumSimulator(size_t num_qubits)
    : num_qubits_(num_qubits), num_amplitudes_(1ULL << num_qubits) {
    state_vector_.resize(num_amplitudes_, complex_t{0, 0});
    state_vector_[0] = complex_t{1, 0};
}

QuantumSimulator::~QuantumSimulator() {}

void QuantumSimulator::Reset() {
    std::fill(state_vector_.begin(), state_vector_.end(), complex_t{0, 0});
    state_vector_[0] = complex_t{1, 0};
    circuit_.clear();
}

void QuantumSimulator::AppendGate(const GateOp& gate) {
    circuit_.push_back(gate);
}

void QuantumSimulator::OptimizeDAG() {
    // Static DAG optimization pass
}

void QuantumSimulator::Run() {
    for (const auto& gate : circuit_) {
        if (gate.type == GateType::H) {
            float inv_sqrt2 = 1.0f / std::sqrt(2.0f);
            GateMatrix2x2 mat{
                {inv_sqrt2, 0}, {inv_sqrt2, 0},
                {inv_sqrt2, 0}, {-inv_sqrt2, 0}
            };
            SIMDComplex::ApplyGate1Q_AVX2(state_vector_.data(), num_amplitudes_, gate.targets[0], mat);
        } else if (gate.type == GateType::X) {
            GateMatrix2x2 mat{
                {0, 0}, {1, 0},
                {1, 0}, {0, 0}
            };
            SIMDComplex::ApplyGate1Q_AVX2(state_vector_.data(), num_amplitudes_, gate.targets[0], mat);
        } else if (gate.type == GateType::CX) {
            GateMatrix4x4 mat{};
            mat.data[0][0] = {1, 0};
            mat.data[1][1] = {1, 0};
            mat.data[2][3] = {1, 0};
            mat.data[3][2] = {1, 0};
            SIMDComplex::ApplyGate2Q_AVX2(state_vector_.data(), num_amplitudes_, gate.targets[0], gate.controls[0], mat);
        }
    }
}

complex_t QuantumSimulator::GetAmplitude(size_t index) const {
    if (index < num_amplitudes_) return state_vector_[index];
    return {0, 0};
}

float QuantumSimulator::GetProbability(size_t index) const {
    if (index < num_amplitudes_) {
        auto amp = state_vector_[index];
        return amp.real() * amp.real() + amp.imag() * amp.imag();
    }
    return 0.0f;
}

std::vector<uint32_t> QuantumSimulator::Sample(size_t num_shots) {
    std::vector<float> cumulative(num_amplitudes_);
    float sum = 0.0f;
    for (size_t i = 0; i < num_amplitudes_; ++i) {
        sum += GetProbability(i);
        cumulative[i] = sum;
    }

    std::vector<uint32_t> shots;
    shots.reserve(num_shots);
    std::default_random_engine rng(42);
    std::uniform_real_distribution<float> dist(0.0f, sum > 0.0f ? sum : 1.0f);

    for (size_t s = 0; s < num_shots; ++s) {
        float r = dist(rng);
        auto it = std::lower_bound(cumulative.begin(), cumulative.end(), r);
        shots.push_back(static_cast<uint32_t>(std::distance(cumulative.begin(), it)));
    }
    return shots;
}

} // namespace quantum
EOF

cd /app

mkdir -p build && cd build
cmake .. -GNinja -DCMAKE_BUILD_TYPE=Release
ninja

echo "=== [Odyssey Oracle] Quantum Simulator Reference Built Successfully ==="
