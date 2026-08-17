#include "wasm_parser.hpp"
#include <cassert>
#include <iostream>

void TestLEB128() {
    std::vector<uint8_t> data = {0xE5, 0x8E, 0x26}; // 624485
    std::span<const uint8_t> sp(data);
    uint32_t val = wasm::BinaryParser::ReadVarUint32(sp);
    assert(val == 624485);
    std::cout << "TestLEB128 passed!" << std::endl;
}

int main() {
    TestLEB128();
    return 0;
}
