#pragma once
#include "quantum_types.hpp"
#include <immintrin.h>

namespace quantum {

class SIMDComplex {
public:
    static void ApplyGate1Q_AVX2(
        complex_t* state,
        size_t num_amplitudes,
        qubit_t target,
        const GateMatrix2x2& mat
    );

    static void ApplyGate2Q_AVX2(
        complex_t* state,
        size_t num_amplitudes,
        qubit_t q0,
        qubit_t q1,
        const GateMatrix4x4& mat
    );
};

} // namespace quantum
