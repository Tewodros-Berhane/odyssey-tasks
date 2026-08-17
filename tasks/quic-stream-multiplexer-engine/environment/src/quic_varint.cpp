#include "quic_varint.hpp"

namespace quic {

size_t VarInt::EncodedLength(uint64_t val) {
    if (val <= 63) return 1;
    if (val <= 16383) return 2;
    if (val <= 1073741823) return 4;
    return 8;
}

size_t VarInt::Encode(uint64_t val, std::span<uint8_t> dst) {
    size_t len = EncodedLength(val);
    if (dst.size() < len) return 0;

    if (len == 1) {
        dst[0] = static_cast<uint8_t>(val);
    } else if (len == 2) {
        dst[0] = 0x40 | static_cast<uint8_t>((val >> 8) & 0x3F);
        dst[1] = static_cast<uint8_t>(val & 0xFF);
    } else if (len == 4) {
        dst[0] = 0x80 | static_cast<uint8_t>((val >> 24) & 0x3F);
        dst[1] = static_cast<uint8_t>((val >> 16) & 0xFF);
        dst[2] = static_cast<uint8_t>((val >> 8) & 0xFF);
        dst[3] = static_cast<uint8_t>(val & 0xFF);
    } else {
        dst[0] = 0xC0 | static_cast<uint8_t>((val >> 56) & 0x3F);
        dst[1] = static_cast<uint8_t>((val >> 48) & 0xFF);
        dst[2] = static_cast<uint8_t>((val >> 40) & 0xFF);
        dst[3] = static_cast<uint8_t>((val >> 32) & 0xFF);
        dst[4] = static_cast<uint8_t>((val >> 24) & 0xFF);
        dst[5] = static_cast<uint8_t>((val >> 16) & 0xFF);
        dst[6] = static_cast<uint8_t>((val >> 8) & 0xFF);
        dst[7] = static_cast<uint8_t>(val & 0xFF);
    }
    return len;
}

std::optional<std::pair<uint64_t, size_t>> VarInt::Decode(std::span<const uint8_t> src) {
    if (src.empty()) return std::nullopt;
    uint8_t prefix = (src[0] >> 6) & 0x03;
    size_t len = 1ULL << prefix;
    if (src.size() < len) return std::nullopt;

    uint64_t val = src[0] & 0x3F;
    for (size_t i = 1; i < len; ++i) {
        val = (val << 8) | src[i];
    }
    return std::make_pair(val, len);
}

} // namespace quic
