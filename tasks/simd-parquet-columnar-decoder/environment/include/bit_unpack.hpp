#pragma once
#include <cstdint>
#include <span>
#include <vector>

namespace parquet {

class BitUnpackSIMD {
public:
    static void Unpack8_AVX2(std::span<const uint8_t> in, uint32_t bit_width, std::span<uint32_t> out);
    static void Unpack32_AVX2(std::span<const uint8_t> in, uint32_t bit_width, std::span<uint32_t> out);
    static void DecodeRLE(std::span<const uint8_t> in, uint32_t bit_width, std::vector<uint32_t>& out, size_t total_vals);
};

} // namespace parquet
