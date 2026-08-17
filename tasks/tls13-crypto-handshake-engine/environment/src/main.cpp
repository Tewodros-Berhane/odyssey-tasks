#include "tls_handshake.hpp"
#include <iostream>

int main() {
    std::cout << "Starting Zero-Dependency TLS 1.3 Engine..." << std::endl;
    tls13::HandshakeEngine client(false);
    auto ch = client.CreateClientHello();
    std::cout << "Generated ClientHello (" << ch.size() << " bytes)." << std::endl;
    return 0;
}
