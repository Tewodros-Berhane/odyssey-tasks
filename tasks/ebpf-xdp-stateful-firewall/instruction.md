# Stateful eBPF/XDP High-Speed Packet Filter with TCP Conntrack and Rate Limiting

## Overview
Your objective is to implement a high-performance in-kernel stateful network firewall using Linux eBPF and XDP (eXpress Data Path) with a user-space control plane daemon in C/C++ in `/app`.

## Architecture & Requirements

### 1. eBPF / XDP In-Kernel Packet Parser
- Hook into XDP layer (`xdp_md` context) for zero-copy early packet processing before kernel SKB allocation.
- Parse nested headers safely with strict boundary checks:
  - Ethernet (`ethhdr`)
  - IPv4 (`iphdr`) & IPv6 (`ipv6hdr`)
  - Transport headers: TCP (`tcphdr`), UDP (`udphdr`), ICMP (`icmphdr`)
- Return XDP action codes: `XDP_PASS`, `XDP_DROP`, `XDP_TX`, or `XDP_ABORTED`.

### 2. Stateful TCP Connection Tracking (Conntrack)
- Implement a 5-tuple flow key (`src_ip`, `dst_ip`, `src_port`, `dst_port`, `protocol`).
- Maintain flow state in `BPF_MAP_TYPE_LRU_HASH` or `BPF_MAP_TYPE_HASH`:
  - States: `SYN_SENT`, `SYN_RECV`, `ESTABLISHED`, `FIN_WAIT_1`, `FIN_WAIT_2`, `TIME_WAIT`, `CLOSED`.
  - Validate TCP sequence numbers and ACK progressions.
  - Reject unexpected flags (e.g., data packets without prior `SYN` handshake or unsolicited `SYN-ACK` responses).
  - Track timestamps for idle connection expiration.

### 3. Dynamic Token-Bucket Rate Limiter
- Implement per-CIDR subnet rate limiting using BPF maps (`BPF_MAP_TYPE_LPM_TRIE` or hash map).
- Store token bucket parameters (`tokens`, `last_updated_ns`, `rate_bytes_per_sec`, `burst_capacity`).
- Atomically decrement tokens per arriving packet/byte; return `XDP_DROP` when bucket is exhausted.

### 4. Stateless SYN Cookie Flood Protection
- When active half-open connection count exceeds a configurable threshold, activate SYN cookie mode.
- Compute cryptographic SYN cookie sequence number based on SipHash / SHA256 of 5-tuple, secret seed, and MSS index.
- Validate ACK packets completing handshake without allocating table state for initial `SYN` packets.

## Build and Test Instructions
The project uses CMake and Clang BPF target:
```bash
cd /app
mkdir -p build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build . --parallel $(nproc)
ctest --output-on-failure
```

The verifier executes `tests/test.sh`, which loads the eBPF bytecode via `libbpf`, attaches it to virtual network namespace interfaces (`veth`), and runs traffic generators for stateful TCP, SYN floods, and rate limits.
