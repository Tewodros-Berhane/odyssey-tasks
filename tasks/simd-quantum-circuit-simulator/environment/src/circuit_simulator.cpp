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
    // TODO: Implement static unitary gate fusion optimizer
}

void QuantumSimulator::Run() {
    // TODO: Implement circuit execution using AVX2 SIMD Complex kernels
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
