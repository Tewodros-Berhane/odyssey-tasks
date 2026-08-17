#pragma once
#include <cstdint>
#include <vector>
#include <span>
#include <string>
#include <variant>
#include <optional>

namespace quic {

enum class FrameType : uint64_t {
    PADDING = 0x00,
    PING = 0x01,
    ACK = 0x02,
    ACK_ECN = 0x03,
    RESET_STREAM = 0x04,
    STOP_SENDING = 0x05,
    CRYPTO = 0x06,
    NEW_TOKEN = 0x07,
    STREAM_BASE = 0x08,
    MAX_DATA = 0x10,
    MAX_STREAM_DATA = 0x11,
    MAX_STREAMS_BIDI = 0x12,
    MAX_STREAMS_UNI = 0x13,
    DATA_BLOCKED = 0x14,
    STREAM_DATA_BLOCKED = 0x15,
    STREAMS_BLOCKED_BIDI = 0x16,
    STREAMS_BLOCKED_UNI = 0x17,
    NEW_CONNECTION_ID = 0x18,
    RETIRE_CONNECTION_ID = 0x19,
    PATH_CHALLENGE = 0x1a,
    PATH_RESPONSE = 0x1b,
    CONNECTION_CLOSE_QUIC = 0x1c,
    CONNECTION_CLOSE_APP = 0x1d,
    HANDSHAKE_DONE = 0x1e
};

struct StreamFrame {
    uint64_t stream_id{0};
    uint64_t offset{0};
    bool fin{false};
    std::vector<uint8_t> data;
};

struct AckRange {
    uint64_t gap{0};
    uint64_t ack_range_length{0};
};

struct AckFrame {
    uint64_t largest_acknowledged{0};
    uint64_t ack_delay{0};
    uint64_t first_ack_range{0};
    std::vector<AckRange> additional_ranges;
};

struct MaxDataFrame {
    uint64_t maximum_data{0};
};

struct MaxStreamDataFrame {
    uint64_t stream_id{0};
    uint64_t maximum_stream_data{0};
};

struct ResetStreamFrame {
    uint64_t stream_id{0};
    uint64_t error_code{0};
    uint64_t final_size{0};
};

using Frame = std::variant<StreamFrame, AckFrame, MaxDataFrame, MaxStreamDataFrame, ResetStreamFrame>;

class FrameParser {
public:
    static std::optional<std::pair<Frame, size_t>> ParseFrame(std::span<const uint8_t> buffer);
    static std::vector<uint8_t> SerializeFrame(const Frame& frame);
};

} // namespace quic
