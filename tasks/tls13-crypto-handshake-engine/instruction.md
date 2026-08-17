# Zero-Dependency RFC 8446 TLS 1.3 Handshake State Machine & AEAD Record Layer

## Overview
Your objective is to implement a standalone, zero-dependency RFC 8446 TLS 1.3 cryptographic handshake engine and AEAD record layer in C++20 in `/app`.

## Architecture & Requirements

### 1. Key Derivation & HKDF Key Schedule (RFC 8446 Section 7.1)
Implement the full TLS 1.3 cryptographic key schedule using HMAC-SHA256:
- `HKDF-Extract(salt, IKM)`
- `HKDF-Expand-Label(Secret, Label, Context, Length)`:
  - Formats `tls13 ` prefix label encoding: `struct { uint16 length; opaque label<7..255>; opaque context<0..255>; } HkdfLabel`.
- Key Schedule States:
  - **Early Secret**: `HKDF-Extract(0, PSK)` $\to$ `c_e_traffic`, `early_exporter_master_secret`.
  - **Handshake Secret**: `HKDF-Extract(derived_secret, (ECDHE_shared_secret))` $\to$ `c_hs_traffic`, `s_hs_traffic`.
  - **Master Secret**: `HKDF-Extract(derived_secret, 0)` $\to$ `c_ap_traffic`, `s_ap_traffic`, `resumption_master_secret`.

### 2. TLS 1.3 Handshake State Machine
Implement client and server state machines handling fragmented and batched handshake records:
- `ClientHello` (Extensions: `supported_versions`, `key_share` with X25519 public key, `signature_algorithms`, `pre_shared_key`).
- `ServerHello` (Cipher suite selection, `key_share`).
- `EncryptedExtensions`.
- `Certificate` & `CertificateVerify` (Signature verification over transcript hash).
- `Finished` (HMAC authentication verification using `finished_key`).
- `KeyUpdate` and Post-Handshake NewSessionTicket generation.

### 3. AEAD Record Layer Framing & Decryption (RFC 8446 Section 5)
- Record header: `ContentType` (0x17 for application data), legacy version `0x0303`, length.
- Plaintext payload framing: `inner_plaintext = payload + inner_content_type + zeros_padding`.
- AEAD Nonce derivation: `nonce = write_iv ^ write_sequence_number` (64-bit implicit monotonic counter).
- Authenticated Additional Data (AAD): 5-byte TLS record header.
- Cipher suites: `TLS_AES_128_GCM_SHA256` and `TLS_CHACHA20_POLY1305_SHA256`.

## Build and Test Instructions
The project uses CMake and C++20:
```bash
cd /app
mkdir -p build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build . --parallel $(nproc)
ctest --output-on-failure
```

The verifier executes `tests/test.sh`, which tests HKDF derivation against RFC 8446 Appendix vectors, 1-RTT/0-RTT handshake loopbacks, fuzz resilience, and constant-time execution.
