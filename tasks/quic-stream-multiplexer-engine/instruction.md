# Zero-Copy RFC 9000 QUIC Packet Demuxer and Flow Controller

## Overview
Your objective is to implement a fast, zero-copy RFC 9000 / RFC 9002 QUIC packet parser, stream multiplexer, and flow controller in C++20 located in `/app`.

## Specifications & Requirements

### 1. RFC 9000 Variable-Length Integer Encoding & Decoding (Varint)
Implement the 2-bit prefix encoded variable-length integer format according to RFC 9000 Section 16:
- `00` (1 byte): 0 to 63 ($2^6 - 1$)
- `01` (2 bytes): 0 to 16383 ($2^{14} - 1$)
- `10` (4 bytes): 0 to 1073741823 ($2^{30} - 1$)
- `11` (8 bytes): 0 to 4611686018427387903 ($2^{62} - 1$)
Ensure zero memory allocations and safe boundary checks against buffer underflow.

### 2. QUIC Packet Demuxing & Frame Parsing
Parse incoming raw UDP packet buffers into structured frames:
- **Long Header & Short Header Parsing**: Extract Connection ID (CID), Packet Number, and payload.
- **Frame Types**:
  - `PADDING` (0x00)
  - `PING` (0x01)
  - `ACK` (0x02 - 0x03): Largest Acknowledged, ACK Delay, ACK Range Count, First ACK Range, and subsequent ACK Ranges.
  - `RESET_STREAM` (0x04): Stream ID, Application Error Code, Final Size.
  - `STOP_SENDING` (0x05): Stream ID, Application Error Code.
  - `MAX_DATA` (0x10): Maximum Data limit.
  - `MAX_STREAM_DATA` (0x11): Stream ID, Maximum Stream Data limit.
  - `MAX_STREAMS` (0x12 - 0x13): Max streams count for Bidi/Uni.
  - `DATA_BLOCKED` (0x14) / `STREAM_DATA_BLOCKED` (0x15).
  - `STREAM` frames (0x08 - 0x0f): Flags for `OFF` (offset present), `LEN` (length present), and `FIN` (stream finished).

### 3. Zero-Copy Stream Assembler (Reassembly Buffer)
- Collect non-contiguous, out-of-order, and overlapping `STREAM` frame slices.
- Maintain an interval-map / contiguous segment range tracker.
- Deliver contiguous bytes to the stream reader without copying chunks until ready for consumption.
- Detect duplicate chunks and truncated/invalid stream final sizes (`FIN` mismatch).

### 4. Connection & Stream Flow Control
- Maintain connection-level byte budget (`max_data_limit` and `bytes_received`).
- Maintain stream-level byte budget (`max_stream_data_limit` and `stream_bytes_received`).
- Generate `MAX_DATA` and `MAX_STREAM_DATA` frame updates when available window falls below 50% capacity.
- Block sending when flow limits are exceeded until credit frames arrive.

## Build and Test Instructions
The project uses CMake and C++20:
```bash
cd /app
mkdir -p build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build . --parallel $(nproc)
ctest --output-on-failure
```

The verifier executes `tests/test.sh`, which grades frame parsing, stream reassembly under jitter/loss, flow control limits, and fuzz resilience.
