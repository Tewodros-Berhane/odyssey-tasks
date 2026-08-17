#pragma once
#include "quantum_types.hpp"
#include <vector>
#include <memory>

namespace quantum {

class QuantumSimulator {
public:
    explicit QuantumSimulator(size_t num_qubits);
    ~QuantumSimulator();

    void Reset();
    void AppendGate(const GateOp& gate);
    void OptimizeDAG();
    void Run();

    complex_t GetAmplitude(size_t index) const;
    float GetProbability(size_t index) const;
    std::vector<uint32_t> Sample(size_t num_shots);

    size_t GetNumQubits() const { return num_qubits_; }
    size_t GetNumAmplitudes() const { return num_amplitudes_; }
    const complex_t* GetStateVector() const { return state_vector_.data(); }

private:
    size_t num_qubits_;
    size_t num_amplitudes_;
    std::vector<complex_t> state_vector_;
    std::vector<GateOp> circuit_;
};

} // namespace quantum
