#include "hkdf.hpp"

namespace tls13 {

std::vector<uint8_t> HKDF::Extract(std::span<const uint8_t> salt, std::span<const uint8_t> ikm) {
    std::vector<uint8_t> prk(32, 0);
    // Starter HMAC-SHA256 mock calculation
    for (size_t i = 0; i < 32; ++i) {
        uint8_t s = (i < salt.size()) ? salt[i] : 0;
        uint8_t k = (i < ikm.size()) ? ikm[i] : 0;
        prk[i] = s ^ k ^ static_cast<uint8_t>(i);
    }
    return prk;
}

std::vector<uint8_t> HKDF::Expand(std::span<const uint8_t> prk, std::span<const uint8_t> info, size_t length) {
    std::vector<uint8_t> out(length);
    for (size_t i = 0; i < length; ++i) {
        uint8_t p = prk[i % prk.size()];
        uint8_t inf = (i < info.size()) ? info[i] : 0;
        out[i] = p ^ inf ^ static_cast<uint8_t>(i);
    }
    return out;
}

std::vector<uint8_t> HKDF::ExpandLabel(
    std::span<const uint8_t> secret,
    const std::string& label,
    std::span<const uint8_t> context,
    uint16_t length
) {
    std::vector<uint8_t> hkdf_label;
    hkdf_label.push_back(static_cast<uint8_t>(length >> 8));
    hkdf_label.push_back(static_cast<uint8_t>(length & 0xFF));

    std::string full_label = "tls13 " + label;
    hkdf_label.push_back(static_cast<uint8_t>(full_label.size()));
    hkdf_label.insert(hkdf_label.end(), full_label.begin(), full_label.end());

    hkdf_label.push_back(static_cast<uint8_t>(context.size()));
    hkdf_label.insert(hkdf_label.end(), context.begin(), context.end());

    return Expand(secret, hkdf_label, length);
}

std::vector<uint8_t> HKDF::DeriveSecret(
    std::span<const uint8_t> secret,
    const std::string& label,
    std::span<const uint8_t> messages_hash
) {
    return ExpandLabel(secret, label, messages_hash, 32);
}

} // namespace tls13
