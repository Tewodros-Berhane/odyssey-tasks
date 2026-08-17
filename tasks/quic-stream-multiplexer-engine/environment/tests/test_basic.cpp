#include "quic_varint.hpp"
#include "quic_frame.hpp"
#include "quic_stream.hpp"
#include "quic_engine.hpp"
#include <cassert>
#include <iostream>

void TestVarInt() {
    uint8_t buf[16];
    size_t len1 = quic::VarInt::Encode(37, buf);
    assert(len1 == 1);
    auto res1 = quic::VarInt::Decode(std::span(buf, len1));
    assert(res1.has_value() && res1->first == 37);

    size_t len2 = quic::VarInt::Encode(15293, buf);
    assert(len2 == 2);
    auto res2 = quic::VarInt::Decode(std::span(buf, len2));
    assert(res2.has_value() && res2->first == 15293);

    std::cout << "TestVarInt passed!" << std::endl;
}

int main() {
    TestVarInt();
    return 0;
}
