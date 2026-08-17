#pragma once
#include "quic_frame.hpp"
#include <map>
#include <vector>
#include <cstdint>
#include <optional>

namespace quic {

class StreamReassembler {
public:
    explicit StreamReassembler(uint64_t stream_id);

    bool PushFrame(const StreamFrame& frame);
    std::vector<uint8_t> Read(size_t max_bytes);
    bool IsFinished() const { return fin_received_ && bytes_read_ == final_size_; }
    uint64_t GetBytesRead() const { return bytes_read_; }

private:
    uint64_t stream_id_;
    uint64_t bytes_read_{0};
    bool fin_received_{false};
    uint64_t final_size_{UINT64_MAX};
    std::map<uint64_t, std::vector<uint8_t>> segments_; // offset -> payload
};

} // namespace quic
