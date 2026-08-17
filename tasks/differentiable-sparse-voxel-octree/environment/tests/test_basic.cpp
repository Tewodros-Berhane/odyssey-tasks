#include "morton.hpp"
#include "volume_renderer.hpp"
#include <cassert>
#include <iostream>

void TestMortonEncoding() {
    uint32_t x = 5, y = 9, z = 12;
    uint64_t code = svo::Morton::Encode3D(x, y, z);
    uint32_t dx = 0, dy = 0, dz = 0;
    svo::Morton::Decode3D(code, dx, dy, dz);

    assert(x == dx);
    assert(y == dy);
    assert(z == dz);

    std::cout << "TestMortonEncoding passed!" << std::endl;
}

int main() {
    TestMortonEncoding();
    return 0;
}
