#pragma once
#include "firewall_types.hpp"
#include "conntrack.hpp"
#include "rate_limiter.hpp"
#include <memory>

namespace firewall {

class FirewallEngine {
public:
    FirewallEngine();

    XdpAction ProcessPacket(const PacketMeta& pkt);
    std::shared_ptr<ConntrackTable> GetConntrack() { return conntrack_; }
    std::shared_ptr<RateLimiter> GetRateLimiter() { return rate_limiter_; }

private:
    std::shared_ptr<ConntrackTable> conntrack_;
    std::shared_ptr<RateLimiter> rate_limiter_;
};

} // namespace firewall
