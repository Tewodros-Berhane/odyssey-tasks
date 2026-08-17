#include "quic_frame.hpp"
#include "quic_varint.hpp"

namespace quic {

std::optional<std::pair<Frame, size_t>> FrameParser::ParseFrame(std::span<const uint8_t> buffer) {
    if (buffer.empty()) return std::nullopt;

    auto type_res = VarInt::Decode(buffer);
    if (!type_res) return std::nullopt;

    uint64_t type_val = type_res->first;
    size_t offset = type_res->second;

    if (type_val >= 0x08 && type_val <= 0x0F) { // STREAM Frame
        bool has_off = (type_val & 0x04) != 0;
        bool has_len = (type_val & 0x02) != 0;
        bool fin = (type_val & 0x01) != 0;

        auto sid_res = VarInt::Decode(buffer.subspan(offset));
        if (!sid_res) return std::nullopt;
        uint64_t stream_id = sid_res->first;
        offset += sid_res->second;

        uint64_t stream_offset = 0;
        if (has_off) {
            auto off_res = VarInt::Decode(buffer.subspan(offset));
            if (!off_res) return std::nullopt;
            stream_offset = off_res->first;
            offset += off_res->second;
        }

        uint64_t length = 0;
        if (has_len) {
            auto len_res = VarInt::Decode(buffer.subspan(offset));
            if (!len_res) return std::nullopt;
            length = len_res->first;
            offset += len_res->second;
        } else {
            length = buffer.size() - offset;
        }

        if (buffer.size() < offset + length) return std::nullopt;

        StreamFrame sf;
        sf.stream_id = stream_id;
        sf.offset = stream_offset;
        sf.fin = fin;
        sf.data.assign(buffer.begin() + offset, buffer.begin() + offset + length);
        offset += length;

        return std::make_pair(sf, offset);
    } else if (type_val == static_cast<uint64_t>(FrameType::MAX_DATA)) {
        auto max_data_res = VarInt::Decode(buffer.subspan(offset));
        if (!max_data_res) return std::nullopt;
        offset += max_data_res->second;
        return std::make_pair(MaxDataFrame{max_data_res->first}, offset);
    } else if (type_val == static_cast<uint64_t>(FrameType::MAX_STREAM_DATA)) {
        auto sid_res = VarInt::Decode(buffer.subspan(offset));
        if (!sid_res) return std::nullopt;
        offset += sid_res->second;
        auto max_sd_res = VarInt::Decode(buffer.subspan(offset));
        if (!max_sd_res) return std::nullopt;
        offset += max_sd_res->second;
        return std::make_pair(MaxStreamDataFrame{sid_res->first, max_sd_res->first}, offset);
    }

    return std::nullopt;
}

std::vector<uint8_t> FrameParser::SerializeFrame(const Frame& frame) {
    std::vector<uint8_t> out;
    if (std::holds_alternative<MaxDataFrame>(frame)) {
        const auto& mf = std::get<MaxDataFrame>(frame);
        uint8_t buf[16];
        size_t l1 = VarInt::Encode(static_cast<uint64_t>(FrameType::MAX_DATA), buf);
        out.insert(out.end(), buf, buf + l1);
        size_t l2 = VarInt::Encode(mf.maximum_data, buf);
        out.insert(out.end(), buf, buf + l2);
    }
    return out;
}

} // namespace quic
