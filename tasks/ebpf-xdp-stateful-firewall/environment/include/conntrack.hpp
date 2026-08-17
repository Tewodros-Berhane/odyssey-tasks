#pragma once
#include "firewall_types.hpp"
#include <unordered_map>
#include <shared_mutex>

namespace firewall {

struct ConntrackEntry {
    TcpState state{TcpState::CLOSED};
    uint32_t last_seq{0};
    uint32_t last_ack{0};
    uint64_t last_seen_ns{0};
};

class ConntrackTable {
public:
    explicit ConntrackTable(size_t max_entries = 1000000);

    bool ProcessTcpPacket(const PacketMeta& pkt);
    void CleanupExpired(uint64_t current_time_ns, uint64_t timeout_ns);
    size_t Size() const;

private:
    struct TupleHash {
        size_t operator()(const FiveTuple& t) const {
            return t.src_ip ^ (t.dst_ip << 1) ^ (t.src_port << 16) ^ t.dst_port;
        }
    };

    size_t max_entries_;
    mutable std::shared_mutex table_mutex_;
    std::unordered_map<FiveTuple, ConntrackEntry, TupleHash> entries_;
};

} // namespace firewall
