#include "quic_stream.hpp"
#include <algorithm>

namespace quic {

StreamReassembler::StreamReassembler(uint64_t stream_id)
    : stream_id_(stream_id) {}

bool StreamReassembler::PushFrame(const StreamFrame& frame) {
    if (frame.stream_id != stream_id_) return false;
    if (frame.fin) {
        fin_received_ = true;
        final_size_ = frame.offset + frame.data.size();
    }
    if (!frame.data.empty()) {
        segments_[frame.offset] = frame.data;
    }
    return true;
}

std::vector<uint8_t> StreamReassembler::Read(size_t max_bytes) {
    std::vector<uint8_t> result;
    while (!segments_.empty() && result.size() < max_bytes) {
        auto it = segments_.begin();
        if (it->first <= bytes_read_) {
            uint64_t seg_start = it->first;
            const auto& data = it->second;
            uint64_t seg_end = seg_start + data.size();

            if (seg_end > bytes_read_) {
                size_t offset_in_data = bytes_read_ - seg_start;
                size_t available = data.size() - offset_in_data;
                size_t to_copy = std::min(available, max_bytes - result.size());

                result.insert(result.end(), data.begin() + offset_in_data, data.begin() + offset_in_data + to_copy);
                bytes_read_ += to_copy;

                if (offset_in_data + to_copy < data.size()) {
                    break;
                }
            }
            segments_.erase(it);
        } else {
            break;
        }
    }
    return result;
}

} // namespace quic
