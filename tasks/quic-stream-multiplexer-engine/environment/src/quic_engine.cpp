#include "quic_engine.hpp"

namespace quic {

QuicEngine::QuicEngine(uint64_t initial_max_data, uint64_t initial_max_stream_data)
    : max_data_(initial_max_data), initial_max_stream_data_(initial_max_stream_data) {}

std::shared_ptr<StreamReassembler> QuicEngine::GetStream(uint64_t stream_id) {
    std::unique_lock lock(engine_mutex_);
    auto it = streams_.find(stream_id);
    if (it != streams_.end()) {
        return it->second;
    }
    auto stream = std::make_shared<StreamReassembler>(stream_id);
    streams_[stream_id] = stream;
    return stream;
}

bool QuicEngine::ProcessPacket(std::span<const uint8_t> packet_payload) {
    size_t offset = 0;
    while (offset < packet_payload.size()) {
        auto res = FrameParser::ParseFrame(packet_payload.subspan(offset));
        if (!res) return false;

        const auto& frame = res->first;
        offset += res->second;

        if (std::holds_alternative<StreamFrame>(frame)) {
            const auto& sf = std::get<StreamFrame>(frame);
            auto stream = GetStream(sf.stream_id);
            if (!stream->PushFrame(sf)) return false;
            bytes_received_ += sf.data.size();
        } else if (std::holds_alternative<MaxDataFrame>(frame)) {
            const auto& mf = std::get<MaxDataFrame>(frame);
            max_data_ = std::max(max_data_, mf.maximum_data);
        }
    }
    return true;
}

std::vector<uint8_t> QuicEngine::GenerateControlFrames() {
    std::vector<uint8_t> out;
    std::unique_lock lock(engine_mutex_);
    for (const auto& f : pending_control_frames_) {
        auto bytes = FrameParser::SerializeFrame(f);
        out.insert(out.end(), bytes.begin(), bytes.end());
    }
    pending_control_frames_.clear();
    return out;
}

} // namespace quic
