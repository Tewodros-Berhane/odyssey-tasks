#include "circuit_simulator.hpp"
#include <cassert>
#include <iostream>
#include <cmath>

void TestBellState() {
    quantum::QuantumSimulator sim(2);
    // H on qubit 0
    quantum::GateOp h_gate;
    h_gate.type = quantum::GateType::H;
    h_gate.targets = {0};
    sim.AppendGate(h_gate);

    // CX on qubit 0 -> 1
    quantum::GateOp cx_gate;
    cx_gate.type = quantum::GateType::CX;
    cx_gate.targets = {1};
    cx_gate.controls = {0};
    sim.AppendGate(cx_gate);

    sim.Run();

    float p00 = sim.GetProbability(0); // |00>
    float p11 = sim.GetProbability(3); // |11>
    float p01 = sim.GetProbability(1); // |01>
    float p10 = sim.GetProbability(2); // |10>

    assert(std::abs(p00 - 0.5f) < 1e-4f);
    assert(std::abs(p11 - 0.5f) < 1e-4f);
    assert(p01 < 1e-4f);
    assert(p10 < 1e-4f);

    std::cout << "TestBellState passed!" << std::endl;
}

int main() {
    TestBellState();
    return 0;
}
