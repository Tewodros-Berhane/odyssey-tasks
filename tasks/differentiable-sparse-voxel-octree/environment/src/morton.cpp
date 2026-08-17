#include "morton.hpp"

namespace svo {

uint64_t Morton::Dilate1to3(uint32_t x) {
    uint64_t v = x & 0x1FFFFF; // 21 bits
    v = (v | (v << 32)) & 0x1F00000000FFFFULL;
    v = (v | (v << 16)) & 0x1F0000FF0000FFULL;
    v = (v | (v << 8))  & 0x100F00F00F00F00FULL;
    v = (v | (v << 4))  & 0x10c30c30c30c30c3ULL;
    v = (v | (v << 2))  & 0x1249249249249249ULL;
    return v;
}

uint32_t Morton::Compact3to1(uint64_t x) {
    uint64_t v = x & 0x1249249249249249ULL;
    v = (v ^ (v >> 2))  & 0x10c30c30c30c30c3ULL;
    v = (v ^ (v >> 4))  & 0x100F00F00F00F00FULL;
    v = (v ^ (v >> 8))  & 0x1F0000FF0000FFULL;
    v = (v ^ (v >> 16)) & 0x1F00000000FFFFULL;
    v = (v ^ (v >> 32)) & 0x1FFFFFULL;
    return static_cast<uint32_t>(v);
}

uint64_t Morton::Encode3D(uint32_t x, uint32_t y, uint32_t z) {
    return (Dilate1to3(x)) | (Dilate1to3(y) << 1) | (Dilate1to3(z) << 2);
}

void Morton::Decode3D(uint64_t code, uint32_t& x, uint32_t& y, uint32_t& z) {
    x = Compact3to1(code);
    y = Compact3to1(code >> 1);
    z = Compact3to1(code >> 2);
}

} // namespace svo
