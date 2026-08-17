#include "rate_limiter.hpp"
#include <algorithm>

namespace firewall {

RateLimiter::RateLimiter(double default_rate_pps, double default_burst)
    : default_rate_(default_rate_pps), default_burst_(default_burst) {}

bool RateLimiter::AllowPacket(uint32_t src_ip_subnet, uint64_t timestamp_ns, double cost) {
    std::unique_lock lock(limiter_mutex_);
    auto& b = buckets_[src_ip_subnet];
    if (b.last_update_ns == 0) {
        b.tokens = default_burst_;
        b.max_capacity = default_burst_;
        b.rate_per_sec = default_rate_;
        b.last_update_ns = timestamp_ns;
    } else {
        double elapsed_sec = (timestamp_ns - b.last_update_ns) / 1e9;
        b.tokens = std::min(b.max_capacity, b.tokens + elapsed_sec * b.rate_per_sec);
        b.last_update_ns = timestamp_ns;
    }

    if (b.tokens >= cost) {
        b.tokens -= cost;
        return true;
    }
    return false;
}

} // namespace firewall
