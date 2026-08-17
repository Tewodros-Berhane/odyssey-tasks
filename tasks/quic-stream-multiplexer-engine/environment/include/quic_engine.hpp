#pragma once
#include "quic_frame.hpp"
#include "quic_stream.hpp"
#include <unordered_map>
#include <memory>
#include <shared_mutex>

namespace quic {

class QuicEngine {
public:
    QuicEngine(uint64_t initial_max_data, uint64_t initial_max_stream_data);

    bool ProcessPacket(std::span<const uint8_t> packet_payload);
    std::shared_ptr<StreamReassembler> GetStream(uint64_t stream_id);
    std::vector<uint8_t> GenerateControlFrames();

    uint64_t GetConnectionMaxData() const { return max_data_; }
    uint64_t GetConnectionBytesReceived() const { return bytes_received_; }

private:
    uint64_t max_data_;
    uint64_t initial_max_stream_data_;
    uint64_t bytes_received_{0};
    mutable std::shared_mutex engine_mutex_;
    std::unordered_map<uint64_t, std::shared_ptr<StreamReassembler>> streams_;
    std::vector<Frame> pending_control_frames_;
};

} // namespace quic
