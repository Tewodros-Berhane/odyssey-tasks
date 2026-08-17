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
