#pragma once
#include <cstdint>

namespace svo {

class Morton {
public:
    static uint64_t Encode3D(uint32_t x, uint32_t y, uint32_t z);
    static void Decode3D(uint64_t code, uint32_t& x, uint32_t& y, uint32_t& z);

private:
    static uint64_t Dilate1to3(uint32_t x);
    static uint32_t Compact3to1(uint64_t x);
};

} // namespace svo
