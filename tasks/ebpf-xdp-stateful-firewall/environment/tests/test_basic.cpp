#include "firewall_engine.hpp"
#include <cassert>
#include <iostream>

void TestConntrackHandshake() {
    firewall::FirewallEngine engine;

    firewall::PacketMeta syn_pkt;
    syn_pkt.tuple.src_ip = 0x0A000001; // 10.0.0.1
    syn_pkt.tuple.dst_ip = 0x0A000002; // 10.0.0.2
    syn_pkt.tuple.src_port = 12345;
    syn_pkt.tuple.dst_port = 80;
    syn_pkt.tuple.protocol = 6; // TCP
    syn_pkt.tcp_flags = 0x02; // SYN
    syn_pkt.timestamp_ns = 1000000;

    assert(engine.ProcessPacket(syn_pkt) == firewall::XdpAction::XDP_PASS);
    assert(engine.GetConntrack()->Size() == 1);

    std::cout << "TestConntrackHandshake passed!" << std::endl;
}

int main() {
    TestConntrackHandshake();
    return 0;
}
