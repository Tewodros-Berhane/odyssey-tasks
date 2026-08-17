#include "bit_unpack.hpp"
#include <immintrin.h>

namespace parquet {

void BitUnpackSIMD::Unpack8_AVX2(std::span<const uint8_t> in, uint32_t bit_width, std::span<uint32_t> out) {
    if (bit_width == 0 || in.empty() || out.size() < 8) return;
    uint32_t mask = (1U << bit_width) - 1;
    uint64_t buffer = 0;
    size_t in_bytes = std::min(in.size(), (size_t)8);
    for (size_t i = 0; i < in_bytes; ++i) {
        buffer |= (static_cast<uint64_t>(in[i]) << (i * 8));
    }
    for (size_t i = 0; i < 8; ++i) {
        out[i] = static_cast<uint32_t>((buffer >> (i * bit_width)) & mask);
    }
}

void BitUnpackSIMD::Unpack32_AVX2(std::span<const uint8_t> in, uint32_t bit_width, std::span<uint32_t> out) {
    if (out.size() < 32) return;
    for (size_t g = 0; g < 4; ++g) {
        size_t byte_offset = (g * 8 * bit_width) / 8;
        if (byte_offset >= in.size()) break;
        Unpack8_AVX2(in.subspan(byte_offset), bit_width, out.subspan(g * 8, 8));
    }
}

void BitUnpackSIMD::DecodeRLE(std::span<const uint8_t> in, uint32_t bit_width, std::vector<uint32_t>& out, size_t total_vals) {
    size_t in_idx = 0;
    while (in_idx < in.size() && out.size() < total_vals) {
        uint32_t header = 0;
        int shift = 0;
        while (in_idx < in.size()) {
            uint8_t b = in[in_idx++];
            header |= (b & 0x7F) << shift;
            shift += 7;
            if ((b & 0x80) == 0) break;
        }

        if ((header & 1) == 0) { // RLE run
            uint32_t count = header >> 1;
            uint32_t val = 0;
            size_t val_bytes = (bit_width + 7) / 8;
            for (size_t b = 0; b < val_bytes && in_idx < in.size(); ++b) {
                val |= (static_cast<uint32_t>(in[in_idx++]) << (b * 8));
            }
            for (uint32_t c = 0; c < count && out.size() < total_vals; ++c) {
                out.push_back(val);
            }
        } else { // Bit-packed run
            uint32_t num_groups = header >> 1;
            uint32_t num_values = num_groups * 8;
            alignas(32) uint32_t buf[8];
            for (uint32_t g = 0; g < num_groups && out.size() < total_vals; ++g) {
                Unpack8_AVX2(in.subspan(in_idx), bit_width, buf);
                in_idx += bit_width;
                for (size_t i = 0; i < 8 && out.size() < total_vals; ++i) {
                    out.push_back(buf[i]);
                }
            }
        }
    }
}

} // namespace parquet
