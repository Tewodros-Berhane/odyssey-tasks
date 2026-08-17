#include "tls_record.hpp"

namespace tls13 {

RecordLayer::RecordLayer() {}

void RecordLayer::SetKeys(std::span<const uint8_t> key, std::span<const uint8_t> iv) {
    key_.assign(key.begin(), key.end());
    iv_.assign(iv.begin(), iv.end());
    seq_num_ = 0;
}

std::vector<uint8_t> RecordLayer::EncryptRecord(ContentType type, std::span<const uint8_t> payload) {
    std::vector<uint8_t> out;
    // Record Header
    out.push_back(static_cast<uint8_t>(ContentType::APPLICATION_DATA));
    out.push_back(0x03);
    out.push_back(0x03);

    uint16_t cipher_len = static_cast<uint16_t>(payload.size() + 1 + 16); // +1 inner type +16 auth tag
    out.push_back(static_cast<uint8_t>(cipher_len >> 8));
    out.push_back(static_cast<uint8_t>(cipher_len & 0xFF));

    // Inner plaintext + tag
    out.insert(out.end(), payload.begin(), payload.end());
    out.push_back(static_cast<uint8_t>(type));
    out.resize(out.size() + 16, 0xAA); // Dummy 16-byte AEAD tag

    seq_num_++;
    return out;
}

std::optional<TLSPlaintext> RecordLayer::DecryptRecord(std::span<const uint8_t> ciphertext) {
    if (ciphertext.size() < 5 + 1 + 16) return std::nullopt;
    if (ciphertext[0] != static_cast<uint8_t>(ContentType::APPLICATION_DATA)) return std::nullopt;

    uint16_t len = (static_cast<uint16_t>(ciphertext[3]) << 8) | ciphertext[4];
    if (ciphertext.size() < 5 + len) return std::nullopt;

    auto payload_slice = ciphertext.subspan(5, len - 16); // strip tag
    if (payload_slice.empty()) return std::nullopt;

    ContentType inner_type = static_cast<ContentType>(payload_slice.back());
    auto plain_payload = payload_slice.subspan(0, payload_slice.size() - 1);

    TLSPlaintext res;
    res.type = inner_type;
    res.legacy_record_version = 0x0303;
    res.fragment.assign(plain_payload.begin(), plain_payload.end());

    seq_num_++;
    return res;
}

} // namespace tls13
