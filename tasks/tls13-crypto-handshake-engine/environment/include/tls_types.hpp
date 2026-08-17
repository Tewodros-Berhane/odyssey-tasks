#pragma once
#include <cstdint>
#include <vector>
#include <string>
#include <span>
#include <optional>

namespace tls13 {

enum class ContentType : uint8_t {
    INVALID = 0,
    CHANGE_CIPHER_SPEC = 20,
    ALERT = 21,
    HANDSHAKE = 22,
    APPLICATION_DATA = 23
};

enum class HandshakeType : uint8_t {
    CLIENT_HELLO = 1,
    SERVER_HELLO = 2,
    NEW_SESSION_TICKET = 4,
    END_OF_EARLY_DATA = 5,
    ENCRYPTED_EXTENSIONS = 8,
    CERTIFICATE = 11,
    CERTIFICATE_REQUEST = 13,
    CERTIFICATE_VERIFY = 15,
    FINISHED = 20,
    KEY_UPDATE = 24
};

enum class CipherSuite : uint16_t {
    TLS_AES_128_GCM_SHA256 = 0x1301,
    TLS_AES_256_GCM_SHA384 = 0x1302,
    TLS_CHACHA20_POLY1305_SHA256 = 0x1303
};

struct TLSPlaintext {
    ContentType type;
    uint16_t legacy_record_version{0x0303};
    std::vector<uint8_t> fragment;
};

struct KeySchedule {
    std::vector<uint8_t> client_early_traffic_secret;
    std::vector<uint8_t> client_handshake_traffic_secret;
    std::vector<uint8_t> server_handshake_traffic_secret;
    std::vector<uint8_t> client_application_traffic_secret;
    std::vector<uint8_t> server_application_traffic_secret;
    std::vector<uint8_t> exporter_master_secret;
    std::vector<uint8_t> resumption_master_secret;
};

} // namespace tls13
