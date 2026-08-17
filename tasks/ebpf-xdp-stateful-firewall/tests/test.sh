#!/usr/bin/env bash
set -e

echo "=== [Odyssey Verifier] Starting eBPF/XDP Firewall Grading ==="

TOTAL_SCORE=0
MAX_SCORE=100

cd /app

mkdir -p build && cd build
cmake .. -GNinja -DCMAKE_BUILD_TYPE=Release
ninja

echo "--- Running Phase 1: Conntrack Handshake Verification (25 pts) ---"
if ./tests/unit_tests; then
    echo "Phase 1 Passed: Basic Conntrack"
    TOTAL_SCORE=$((TOTAL_SCORE + 25))
else
    echo "Phase 1 Failed"
fi

echo "--- Running Phase 2: Stateful TCP Transition Stress (25 pts) ---"
cat << 'EOF' > test_tcp_stress.cpp
#include "firewall_engine.hpp"
#include <cassert>
#include <iostream>

int main() {
    firewall::FirewallEngine engine;

    for (uint32_t i = 1; i <= 1000; ++i) {
        firewall::PacketMeta p;
        p.tuple.src_ip = 0x0A000000 | i;
        p.tuple.dst_ip = 0x0A000101;
        p.tuple.src_port = static_cast<uint16_t>(10000 + (i % 50000));
        p.tuple.dst_port = 443;
        p.tuple.protocol = 6;
        p.tcp_flags = 0x02; // SYN
        p.timestamp_ns = i * 1000;

        assert(engine.ProcessPacket(p) == firewall::XdpAction::XDP_PASS);
    }
    assert(engine.GetConntrack()->Size() == 1000);

    std::cout << "Stateful TCP stress test passed!" << std::endl;
    return 0;
}
EOF
g++ -std=c++20 -O3 -I../include test_tcp_stress.cpp libfirewall_engine.a -lpthread -o test_tcp_stress
if ./test_tcp_stress; then
    echo "Phase 2 Passed: Stateful TCP Stress"
    TOTAL_SCORE=$((TOTAL_SCORE + 25))
else
    echo "Phase 2 Failed"
fi

echo "--- Running Phase 3: Token Bucket Rate Limiting (25 pts) ---"
cat << 'EOF' > test_rate_limit.cpp
#include "rate_limiter.hpp"
#include <cassert>
#include <iostream>

int main() {
    firewall::RateLimiter limiter(100.0, 10.0); // 100 pps, burst of 10
    uint32_t subnet = 0x0A000000;

    int passed = 0;
    for (int i = 0; i < 20; ++i) {
        if (limiter.AllowPacket(subnet, 1000)) {
            passed++;
        }
    }
    assert(passed == 10); // Exactly burst capacity allowed
    std::cout << "Token bucket test passed! Allowed: " << passed << std::endl;
    return 0;
}
EOF
g++ -std=c++20 -O3 -I../include test_rate_limit.cpp libfirewall_engine.a -lpthread -o test_rate_limit
if ./test_rate_limit; then
    echo "Phase 3 Passed: Rate Limiter"
    TOTAL_SCORE=$((TOTAL_SCORE + 25))
else
    echo "Phase 3 Failed"
fi

echo "--- Running Phase 4: Sanitizer Pass (25 pts) ---"
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
