#pragma once
#include <complex>
#include <vector>
#include <cstdint>
#include <span>

namespace quantum {

using complex_t = std::complex<float>;
using qubit_t = uint32_t;

struct GateMatrix2x2 {
    complex_t m00{1, 0}, m01{0, 0};
    complex_t m10{0, 0}, m11{1, 0};
};

struct GateMatrix4x4 {
    complex_t data[4][4]{};
};

enum class GateType {
    H,
    X,
    Y,
    Z,
    S,
    T,
    RX,
    RY,
    RZ,
    CX,
    CZ,
    SWAP,
    FUSED_2Q
};

struct GateOp {
    GateType type;
    std::vector<qubit_t> targets;
    std::vector<qubit_t> controls;
    std::vector<float> params;
    GateMatrix2x2 matrix2x2;
    GateMatrix4x4 matrix4x4;
};

} // namespace quantum
