#include "conntrack.hpp"

namespace firewall {

ConntrackTable::ConntrackTable(size_t max_entries)
    : max_entries_(max_entries) {}

bool ConntrackTable::ProcessTcpPacket(const PacketMeta& pkt) {
    std::unique_lock lock(table_mutex_);
    auto it = entries_.find(pkt.tuple);

    uint8_t syn = (pkt.tcp_flags & 0x02) != 0;
    uint8_t ack = (pkt.tcp_flags & 0x10) != 0;
    uint8_t fin = (pkt.tcp_flags & 0x01) != 0;
    uint8_t rst = (pkt.tcp_flags & 0x04) != 0;

    if (it == entries_.end()) {
        if (syn && !ack) { // Initial SYN
            if (entries_.size() >= max_entries_) return false;
            ConntrackEntry entry;
            entry.state = TcpState::SYN_SENT;
            entry.last_seq = pkt.seq;
            entry.last_ack = 0;
            entry.last_seen_ns = pkt.timestamp_ns;
            entries_[pkt.tuple] = entry;
            return true;
        }
        return false;
    }

    auto& entry = it->second;
    entry.last_seen_ns = pkt.timestamp_ns;

    if (rst) {
        entries_.erase(it);
        return true;
    }

    if (entry.state == TcpState::SYN_SENT && syn && ack) {
        entry.state = TcpState::SYN_RECV;
    } else if (entry.state == TcpState::SYN_RECV && ack) {
        entry.state = TcpState::ESTABLISHED;
    } else if (fin) {
        entry.state = TcpState::FIN_WAIT;
    }

    return true;
}

void ConntrackTable::CleanupExpired(uint64_t current_time_ns, uint64_t timeout_ns) {
    std::unique_lock lock(table_mutex_);
    for (auto it = entries_.begin(); it != entries_.end(); ) {
        if (current_time_ns > it->second.last_seen_ns + timeout_ns) {
            it = entries_.erase(it);
        } else {
            ++it;
        }
    }
}

size_t ConntrackTable::Size() const {
    std::shared_lock lock(table_mutex_);
    return entries_.size();
}

} // namespace firewall
