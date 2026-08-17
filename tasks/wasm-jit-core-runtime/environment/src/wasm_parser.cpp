#include "wasm_parser.hpp"

namespace wasm {

uint32_t BinaryParser::ReadVarUint32(std::span<const uint8_t>& stream) {
    uint32_t result = 0;
    int shift = 0;
    while (!stream.empty()) {
        uint8_t byte = stream[0];
        stream = stream.subspan(1);
        result |= (byte & 0x7F) << shift;
        if ((byte & 0x80) == 0) break;
        shift += 7;
    }
    return result;
}

int32_t BinaryParser::ReadVarInt32(std::span<const uint8_t>& stream) {
    int32_t result = 0;
    int shift = 0;
    uint8_t byte = 0;
    while (!stream.empty()) {
        byte = stream[0];
        stream = stream.subspan(1);
        result |= (byte & 0x7F) << shift;
        shift += 7;
        if ((byte & 0x80) == 0) break;
    }
    if ((shift < 32) && (byte & 0x40)) {
        result |= (~0 << shift);
    }
    return result;
}

int64_t BinaryParser::ReadVarInt64(std::span<const uint8_t>& stream) {
    int64_t result = 0;
    int shift = 0;
    uint8_t byte = 0;
    while (!stream.empty()) {
        byte = stream[0];
        stream = stream.subspan(1);
        result |= static_cast<int64_t>(byte & 0x7F) << shift;
        shift += 7;
        if ((byte & 0x80) == 0) break;
    }
    if ((shift < 64) && (byte & 0x40)) {
        result |= (~0ULL << shift);
    }
    return result;
}

std::string BinaryParser::ReadString(std::span<const uint8_t>& stream) {
    uint32_t len = ReadVarUint32(stream);
    if (stream.size() < len) return "";
    std::string s(stream.begin(), stream.begin() + len);
    stream = stream.subspan(len);
    return s;
}

std::optional<Module> BinaryParser::Parse(std::span<const uint8_t> bytes) {
    if (bytes.size() < 8) return std::nullopt;
    // Check magic '\0asm' and version 1
    if (bytes[0] != 0x00 || bytes[1] != 0x61 || bytes[2] != 0x73 || bytes[3] != 0x6D) return std::nullopt;

    Module mod;
    auto stream = bytes.subspan(8);

    while (!stream.empty()) {
        uint8_t sec_id = stream[0];
        stream = stream.subspan(1);
        uint32_t sec_len = ReadVarUint32(stream);
        if (stream.size() < sec_len) return std::nullopt;

        auto sec_payload = stream.subspan(0, sec_len);
        stream = stream.subspan(sec_len);

        if (sec_id == static_cast<uint8_t>(SectionId::TYPE)) {
            uint32_t count = ReadVarUint32(sec_payload);
            for (uint32_t i = 0; i < count; ++i) {
                if (sec_payload.empty() || sec_payload[0] != 0x60) break;
                sec_payload = sec_payload.subspan(1);
                FuncType ft;
                uint32_t param_count = ReadVarUint32(sec_payload);
                for (uint32_t p = 0; p < param_count; ++p) {
                    ft.params.push_back(static_cast<ValType>(sec_payload[0]));
                    sec_payload = sec_payload.subspan(1);
                }
                uint32_t ret_count = ReadVarUint32(sec_payload);
                for (uint32_t r = 0; r < ret_count; ++r) {
                    ft.returns.push_back(static_cast<ValType>(sec_payload[0]));
                    sec_payload = sec_payload.subspan(1);
                }
                mod.types.push_back(ft);
            }
        } else if (sec_id == static_cast<uint8_t>(SectionId::FUNCTION)) {
            uint32_t count = ReadVarUint32(sec_payload);
            for (uint32_t i = 0; i < count; ++i) {
                mod.functions.push_back(ReadVarUint32(sec_payload));
            }
        } else if (sec_id == static_cast<uint8_t>(SectionId::EXPORT)) {
            uint32_t count = ReadVarUint32(sec_payload);
            for (uint32_t i = 0; i < count; ++i) {
                ExportEntry exp;
                exp.name = ReadString(sec_payload);
                exp.kind = sec_payload[0];
                sec_payload = sec_payload.subspan(1);
                exp.index = ReadVarUint32(sec_payload);
                mod.exports.push_back(exp);
            }
        } else if (sec_id == static_cast<uint8_t>(SectionId::CODE)) {
            uint32_t count = ReadVarUint32(sec_payload);
            for (uint32_t i = 0; i < count; ++i) {
                uint32_t body_len = ReadVarUint32(sec_payload);
                auto body_payload = sec_payload.subspan(0, body_len);
                sec_payload = sec_payload.subspan(body_len);

                FunctionBody fb;
                uint32_t local_decls = ReadVarUint32(body_payload);
                for (uint32_t l = 0; l < local_decls; ++l) {
                    uint32_t num = ReadVarUint32(body_payload);
                    ValType vt = static_cast<ValType>(body_payload[0]);
                    body_payload = body_payload.subspan(1);
                    fb.locals.push_back({num, vt});
                }
                fb.code.assign(body_payload.begin(), body_payload.end());
                mod.bodies.push_back(fb);
            }
        }
    }
    return mod;
}

} // namespace wasm
