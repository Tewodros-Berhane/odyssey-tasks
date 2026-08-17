#pragma once
#include <cstdint>
#include <unordered_map>
#include <shared_mutex>

namespace firewall {

struct TokenBucket {
    double tokens{0.0};
    double max_capacity{1000.0};
    double rate_per_sec{1000.0};
    uint64_t last_update_ns{0};
};

class RateLimiter {
public:
    RateLimiter(double default_rate_pps = 1000.0, double default_burst = 2000.0);

    bool AllowPacket(uint32_t src_ip_subnet, uint64_t timestamp_ns, double cost = 1.0);

private:
    double default_rate_;
    double default_burst_;
    mutable std::shared_mutex limiter_mutex_;
    std::unordered_map<uint32_t, TokenBucket> buckets_;
};

} // namespace firewall
