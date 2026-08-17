#pragma once
#include "tls_types.hpp"
#include "tls_record.hpp"
#include "hkdf.hpp"
#include <memory>

namespace tls13 {

class HandshakeEngine {
public:
    explicit HandshakeEngine(bool is_server);

    std::vector<uint8_t> CreateClientHello();
    bool ProcessServerHello(std::span<const uint8_t> msg);
    bool IsHandshakeComplete() const { return complete_; }

    RecordLayer& GetInboundRecord() { return inbound_record_; }
    RecordLayer& GetOutboundRecord() { return outbound_record_; }

private:
    bool is_server_;
    bool complete_{false};
    KeySchedule key_schedule_;
    RecordLayer inbound_record_;
    RecordLayer outbound_record_;
    std::vector<uint8_t> transcript_hash_;
};

} // namespace tls13
