#include "bit_unpack.hpp"
#include <cassert>
#include <iostream>

void TestBitUnpack() {
    // 8 3-bit integers: 1, 2, 3, 4, 5, 6, 7, 0
    // Binary: 000 111 110 101 100 011 010 001
    // Total 24 bits = 3 bytes
    std::vector<uint8_t> in_bytes = {0b10010001, 0b11011100, 0b00000111};
    alignas(32) uint32_t out[8] = {0};

    parquet::BitUnpackSIMD::Unpack8_AVX2(in_bytes, 3, out);
    assert(out[0] == 1);
    assert(out[1] == 2);
    assert(out[2] == 3);
    assert(out[3] == 4);
    assert(out[4] == 5);
    assert(out[5] == 6);
    assert(out[6] == 7);
    assert(out[7] == 0);

    std::cout << "TestBitUnpack passed!" << std::endl;
}

int main() {
    TestBitUnpack();
    return 0;
}
