#include "quic_engine.hpp"
#include "quic_varint.hpp"
#include <iostream>

int main() {
    std::cout << "Starting QUIC Stream Multiplexer & Demuxer Engine..." << std::endl;
    quic::QuicEngine engine(1048576, 65536);
    std::cout << "Engine initialized with MaxData = " << engine.GetConnectionMaxData() << std::endl;
    return 0;
}
