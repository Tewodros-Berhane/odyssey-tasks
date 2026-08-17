#pragma once
#include "tls_types.hpp"

namespace tls13 {

class HKDF {
public:
    static std::vector<uint8_t> Extract(std::span<const uint8_t> salt, std::span<const uint8_t> ikm);
    static std::vector<uint8_t> Expand(std::span<const uint8_t> prk, std::span<const uint8_t> info, size_t length);
    static std::vector<uint8_t> ExpandLabel(
        std::span<const uint8_t> secret,
        const std::string& label,
        std::span<const uint8_t> context,
        uint16_t length
    );
    static std::vector<uint8_t> DeriveSecret(
        std::span<const uint8_t> secret,
        const std::string& label,
        std::span<const uint8_t> messages_hash
    );
};

} // namespace tls13
