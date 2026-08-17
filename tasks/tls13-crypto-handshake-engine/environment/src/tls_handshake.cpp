#include "tls_handshake.hpp"

namespace tls13 {

HandshakeEngine::HandshakeEngine(bool is_server)
    : is_server_(is_server) {}

std::vector<uint8_t> HandshakeEngine::CreateClientHello() {
    std::vector<uint8_t> ch;
    ch.push_back(static_cast<uint8_t>(HandshakeType::CLIENT_HELLO));
    // Length placeholder (3 bytes)
    ch.push_back(0x00);
    ch.push_back(0x00);
    ch.push_back(0x20);

    // Legacy version 0x0303
    ch.push_back(0x03);
    ch.push_back(0x03);

    // Random 32 bytes
    for (int i = 0; i < 32; ++i) ch.push_back(0x42);

    return ch;
}

bool HandshakeEngine::ProcessServerHello(std::span<const uint8_t> msg) {
    if (msg.empty()) return false;
    if (msg[0] != static_cast<uint8_t>(HandshakeType::SERVER_HELLO)) return false;

    // Simulate key schedule derivation
    std::vector<uint8_t> dummy_shared(32, 0x11);
    std::vector<uint8_t> early_sec = HKDF::Extract({}, {});
    std::vector<uint8_t> hs_sec = HKDF::Extract(early_sec, dummy_shared);

    key_schedule_.client_handshake_traffic_secret = HKDF::DeriveSecret(hs_sec, "c hs traffic", {});
    key_schedule_.server_handshake_traffic_secret = HKDF::DeriveSecret(hs_sec, "s hs traffic", {});

    complete_ = true;
    return true;
}

} // namespace tls13
