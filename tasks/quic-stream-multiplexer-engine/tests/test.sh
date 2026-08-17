#!/usr/bin/env bash
set -e

echo "=== [Odyssey Verifier] Starting QUIC Engine Grading ==="

TOTAL_SCORE=0
MAX_SCORE=100

cd /app

mkdir -p build && cd build
cmake .. -GNinja -DCMAKE_BUILD_TYPE=Release
ninja

echo "--- Running Phase 1: VarInt & Frame Decoding (25 pts) ---"
if ./tests/unit_tests; then
    echo "Phase 1 Passed: Varint & basic frames"
    TOTAL_SCORE=$((TOTAL_SCORE + 25))
else
    echo "Phase 1 Failed"
fi

echo "--- Running Phase 2: Out-of-Order Stream Reassembly Stress (25 pts) ---"
cat << 'EOF' > test_reassembly.cpp
#include "quic_stream.hpp"
#include <cassert>
#include <iostream>
#include <vector>
#include <random>
#include <numeric>

int main() {
    quic::StreamReassembler stream(1);
    
    // Generate 100 random chunks out of order
    std::string full_payload = "QUIC_HIGH_PERFORMANCE_TEST_PAYLOAD_";
    for (int i = 0; i < 200; ++i) full_payload += "CHUNK_" + std::to_string(i) + "_DATA_";

    size_t chunk_size = 32;
    std::vector<quic::StreamFrame> frames;
    for (size_t off = 0; off < full_payload.size(); off += chunk_size) {
        size_t len = std::min(chunk_size, full_payload.size() - off);
        quic::StreamFrame sf;
        sf.stream_id = 1;
        sf.offset = off;
        sf.data.assign(full_payload.begin() + off, full_payload.begin() + off + len);
        sf.fin = (off + len == full_payload.size());
        frames.push_back(sf);
    }

    std::mt19937 g(42);
    std::shuffle(frames.begin(), frames.end(), g);

    for (const auto& f : frames) {
        assert(stream.PushFrame(f));
    }

    auto reconstructed_bytes = stream.Read(full_payload.size() + 100);
    std::string reconstructed(reconstructed_bytes.begin(), reconstructed_bytes.end());
    assert(reconstructed == full_payload);
    assert(stream.IsFinished());

    std::cout << "Stream reassembly stress test passed! Reassembled " << reconstructed.size() << " bytes." << std::endl;
    return 0;
}
EOF
g++ -std=c++20 -O3 -I../include test_reassembly.cpp libquic_engine.a -lpthread -o test_reassembly
if ./test_reassembly; then
    echo "Phase 2 Passed: Out-of-order stream reassembly"
    TOTAL_SCORE=$((TOTAL_SCORE + 25))
else
    echo "Phase 2 Failed"
fi

echo "--- Running Phase 3: Flow Control & MaxData Constraints (25 pts) ---"
cat << 'EOF' > test_flow_control.cpp
#include "quic_engine.hpp"
#include "quic_varint.hpp"
#include <cassert>
#include <iostream>

int main() {
    quic::QuicEngine engine(500, 200);
    assert(engine.GetConnectionMaxData() == 500);

    // Feed a frame exceeding stream data
    quic::StreamFrame sf;
    sf.stream_id = 1;
    sf.offset = 0;
    sf.data.resize(300, 'A');
    
    // Simulate serialized stream frame buffer
    std::vector<uint8_t> buf;
    uint8_t tmp[16];
    size_t l1 = quic::VarInt::Encode(0x0E, tmp); // STREAM with OFF and LEN
    buf.insert(buf.end(), tmp, tmp + l1);
    size_t l2 = quic::VarInt::Encode(1, tmp); // Stream ID 1
    buf.insert(buf.end(), tmp, tmp + l2);
    size_t l3 = quic::VarInt::Encode(0, tmp); // Offset 0
    buf.insert(buf.end(), tmp, tmp + l3);
    size_t l4 = quic::VarInt::Encode(300, tmp); // Len 300
    buf.insert(buf.end(), tmp, tmp + l4);
    buf.insert(buf.end(), sf.data.begin(), sf.data.end());

    assert(engine.ProcessPacket(buf));
    assert(engine.GetConnectionBytesReceived() == 300);

    std::cout << "Flow control test passed!" << std::endl;
    return 0;
}
EOF
g++ -std=c++20 -O3 -I../include test_flow_control.cpp libquic_engine.a -lpthread -o test_flow_control
if ./test_flow_control; then
    echo "Phase 3 Passed: Flow Control Verification"
    TOTAL_SCORE=$((TOTAL_SCORE + 25))
else
    echo "Phase 3 Failed"
fi

echo "--- Running Phase 4: Sanitizer and Fuzzing Pass (25 pts) ---"
cmake .. -GNinja -DCMAKE_BUILD_TYPE=Debug -DCMAKE_CXX_FLAGS="-fsanitize=address,undefined -g"
ninja
if ./tests/unit_tests; then
    echo "Phase 4 Passed: ASan & UBsan clear"
    TOTAL_SCORE=$((TOTAL_SCORE + 25))
else
    echo "Phase 4 Failed"
fi

echo "=========================================="
echo "FINAL SCORE: ${TOTAL_SCORE} / ${MAX_SCORE}"
echo "=========================================="

if [ "${TOTAL_SCORE}" -ge 80 ]; then
    echo "VERDICT: SUCCESS"
    exit 0
else
    echo "VERDICT: FAILURE"
    exit 1
fi
