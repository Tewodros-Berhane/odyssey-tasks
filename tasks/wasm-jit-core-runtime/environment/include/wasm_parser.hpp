#pragma once
#include "wasm_types.hpp"

namespace wasm {

class BinaryParser {
public:
    static uint32_t ReadVarUint32(std::span<const uint8_t>& stream);
    static int32_t ReadVarInt32(std::span<const uint8_t>& stream);
    static int64_t ReadVarInt64(std::span<const uint8_t>& stream);
    static std::string ReadString(std::span<const uint8_t>& stream);

    static std::optional<Module> Parse(std::span<const uint8_t> bytes);
};

} // namespace wasm
