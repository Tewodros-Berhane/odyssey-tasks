#pragma once
#include <cstdint>
#include <cstddef>
#include <span>
#include <optional>

namespace quic {

struct VarInt {
    static size_t EncodedLength(uint64_t val);
    static size_t Encode(uint64_t val, std::span<uint8_t> dst);
    static std::optional<std::pair<uint64_t, size_t>> Decode(std::span<const uint8_t> src);
};

} // namespace quic
