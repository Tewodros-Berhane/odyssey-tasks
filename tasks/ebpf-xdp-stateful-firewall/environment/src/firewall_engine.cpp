#include "firewall_engine.hpp"

namespace firewall {

FirewallEngine::FirewallEngine()
    : conntrack_(std::make_shared<ConntrackTable>()),
      rate_limiter_(std::make_shared<RateLimiter>()) {}

XdpAction FirewallEngine::ProcessPacket(const PacketMeta& pkt) {
    // 1. Rate Limiting Check on Subnet (/24)
    uint32_t subnet = pkt.tuple.src_ip & 0xFFFFFF00;
    if (!rate_limiter_->AllowPacket(subnet, pkt.timestamp_ns)) {
        return XdpAction::XDP_DROP;
    }

    // 2. Conntrack TCP Inspection
    if (pkt.tuple.protocol == 6) { // TCP
        if (!conntrack_->ProcessTcpPacket(pkt)) {
            return XdpAction::XDP_DROP;
        }
    }

    return XdpAction::XDP_PASS;
}

} // namespace firewall
