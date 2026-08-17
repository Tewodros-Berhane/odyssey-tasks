#pragma once
#include "tls_types.hpp"

namespace tls13 {

class RecordLayer {
public:
    RecordLayer();

    void SetKeys(std::span<const uint8_t> key, std::span<const uint8_t> iv);
    std::vector<uint8_t> EncryptRecord(ContentType type, std::span<const uint8_t> payload);
    std::optional<TLSPlaintext> DecryptRecord(std::span<const uint8_t> ciphertext);

private:
    std::vector<uint8_t> key_;
    std::vector<uint8_t> iv_;
    uint64_t seq_num_{0};
};

} // namespace tls13
