#include "circuit_simulator.hpp"
#include <iostream>

int main() {
    std::cout << "Starting SIMD Quantum Circuit Simulator..." << std::endl;
    quantum::QuantumSimulator sim(10);
    std::cout << "Allocated state vector for 10 qubits (" << sim.GetNumAmplitudes() << " amplitudes)." << std::endl;
    return 0;
}
