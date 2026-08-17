#pragma once
#include <cstdint>
#include <vector>
#include <string>
#include <optional>

namespace firewall {

enum class XdpAction : uint32_t {
    XDP_ABORTED = 0,
    XDP_DROP = 1,
    XDP_PASS = 2,
    XDP_TX = 3,
    XDP_REDIRECT = 4
};

enum class TcpState : uint8_t {
    CLOSED = 0,
    SYN_SENT,
    SYN_RECV,
    ESTABLISHED,
    FIN_WAIT,
    TIME_WAIT
};

struct FiveTuple {
    uint32_t src_ip{0};
    uint32_t dst_ip{0};
    uint16_t src_port{0};
    uint16_t dst_port{0};
    uint8_t protocol{0};

    bool operator==(const FiveTuple& o) const {
        return src_ip == o.src_ip && dst_ip == o.dst_ip &&
               src_port == o.src_port && dst_port == o.dst_port &&
               protocol == o.protocol;
    }
};

struct PacketMeta {
    FiveTuple tuple;
    uint8_t tcp_flags{0};
    uint32_t seq{0};
    uint32_t ack{0};
    size_t payload_len{0};
    uint64_t timestamp_ns{0};
};

} // namespace firewall
