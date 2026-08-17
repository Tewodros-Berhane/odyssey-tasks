# Odyssey Task Proposals (Frontier Level)

---

## Task 1: High-Throughput MVCC B-Tree Storage Engine with Write-Ahead Logging & Crash Recovery

### Draft Metadata

- **`title`**: In-Memory MVCC B-Tree Storage Engine with WAL and ARIES Recovery
- **`workingSlug`**: `mvcc-btree-wal-recovery`
- **`collectionFamily`**: `Library clone`
- **`taskFamily`**: `feature_development`
- **`verifierFamily`**: `programmatic`
- **`expertTimeEstimateHours`**: 18

### Problem & Scope
- **`objective`**: Implement a thread-safe, lock-free/latch-crabbing MVCC B+Tree storage engine in C++20 or Rust supporting Snapshot Isolation (SI), optimistic concurrent transactions, Write-Ahead Logging (WAL) with group commit, checkpointing, and deterministic crash recovery conforming to ARIES (Analysis, Redo, Undo). The engine must handle arbitrary concurrently interleaving read-write transactions, rollbacks, phantom-avoidance via key-range locking or MVCC visibility timestamps, and recover completely from simulated ungraceful process terminations (`SIGKILL` during active flushes).
- **`motivation`**: Database kernels and transactional engines require nuanced concurrency control, low-overhead latch management, and bulletproof recovery semantics. Standard frontier models frequently struggle with edge cases in concurrent node splits, dirty read visibility during cascading aborts, and WAL LSN (Log Sequence Number) ordering under crash-recovery scenarios.

### Difficulty & Reasoning
- **`difficultyExplanation`**:
  1. **Concurrent Node Splits & Structural Modifications**: Concurrent latch-crabbing across B+Tree page splits under high contention often deadlocks or leaks inconsistent pointers if optimistic descent fails.
  2. **MVCC GC & LSN Chaining**: Implementing non-blocking vacuuming/garbage collection for expired tuple versions without stalling readers while maintaining monotonic transaction commit timestamps.
  3. **Crash Recovery & Fuzzy Checkpoints**: Implementing true ARIES recovery requires handling interrupted log records, recreating the Dirty Page Table (DPT) and Transaction Table during Analysis, replaying Redo from the minimum RecLSN, and scanning backwards for Undo to roll back uncommitted transactions without corrupting recovered pages.

### Environment & Compute Budget
- **`environmentSummary`**: Debian 12 base image with `gcc-13`, `clang-18`, `cmake`, `ninja-build`, `valgrind`, `liburing-dev`, `cargo`/Rust 1.78, and Python 3.11 test runner harness pre-installed in `/app`.
- **`resourceEstimate`**:
  - `cpuMillis`: 8000 (8 vCPUs)
  - `memoryMb`: 16384 (16 GB)
  - `storageMb`: 10240 (10 GB)
  - `gpuCount`: 0
  - `agentTimeoutSec`: 28800 (8 hours)
  - `verifierTimeoutSec`: 1800 (30 mins)
- **`networkRequirements`**: `none` (fully offline)

### Oracle & Verification
- **`oracleStrategy`**: Complete reference implementation featuring atomic page latching (B-link style right-sibling pointers), commit timestamp allocator, append-only circular WAL buffer with background sync thread, and an ARIES recovery state machine.
- **`verificationStrategy`**:
  - **Public Tests**: Basic CRUD operations, single-thread transactional commit/rollback, and clean restart tests.
  - **Hidden / Sealed Tests**:
    1. Multi-threaded stress test with Jepsen-style transaction histories verifying Snapshot Isolation invariants and serializability anomalies under 64 concurrent workers.
    2. Chaos crash injection: Process killed via `SIGKILL` at random microsecond offsets during high-load write bursts, followed by database restart and automated verification of ACID consistency, zero data corruption, and exact state matches against committed transaction logs.
    3. ThreadSanitizer (TSan) and AddressSanitizer (ASan) verification passes to ensure zero race conditions, deadlocks, or memory leaks.

### Scoring & Anti-Exploits
- **`binarySuccessCondition`**: Engine passes all concurrency stress trials without data loss, achieves 100% ACID compliance under random crash injection, and reports 0 race/memory sanitizer violations.
- **`partialScoreStrategy`**:
  - 20%: Single-threaded transaction processing & basic WAL persistence.
  - 30%: Multi-threaded concurrent read/write throughput without deadlocks.
  - 30%: Deterministic ARIES crash-recovery after abrupt termination.
  - 20%: Strict Snapshot Isolation validation & zero Sanitizer violations.
- **`anticipatedExploits`**:
  - *Exploit*: Disabling crash recovery by synchronously flushing every single write or using an in-memory SQLite wrapper.
  - *Mitigation*: Verifier intercepts system calls, enforces strict throughput floors (>50k txn/sec), injects kernel-level I/O drops, and inspects binary symbols and page header formats directly.

---

## Task 2: Zero-Copy QUIC Protocol Parser & Stream Multiplexer Engine

### Draft Metadata

- **`title`**: Zero-Copy RFC 9000 QUIC Packet Demuxer and Flow Controller
- **`workingSlug`**: `quic-stream-multiplexer-engine`
- **`collectionFamily`**: `Library clone`
- **`taskFamily`**: `feature_development`
- **`verifierFamily`**: `programmatic`
- **`expertTimeEstimateHours`**: 16

### Problem & Scope
- **`objective`**: Implement a high-performance, zero-copy RFC 9000 / RFC 9002 QUIC packet parser, stream multiplexer, and congestion controller from scratch in C or Rust. The engine must handle packet decryption/header protection removal, variable-length integer decoding, ACK frame consolidation, connection migration, stream flow control (STREAM and MAX_STREAM_DATA frames), lost packet retransmission, and NewReno/BBR congestion control.
- **`motivation`**: Modern edge proxies and network appliances rely on custom user-space transport protocols. QUIC combines transport, multiplexing, and cryptographic framing into complex state machines where subtle state desynchronizations lead to silent stream hangs or security bypasses.

### Difficulty & Reasoning
- **`difficultyExplanation`**:
  1. **Packet Number Encoding/Decoding**: Variable-length truncated packet number arithmetic and anti-replay sliding window filters require exact bitwise arithmetic.
  2. **Stream Assembly & Out-of-Order Gaps**: Reassembling fragmented stream frames with overlapping ranges, out-of-order chunks, and zero allocations on the fast path.
  3. **Cryptographic Header Protection & Key Rotation**: Implementing 1-RTT key updates and short header parsing without copying packet payload buffers.

### Environment & Compute Budget
- **`environmentSummary`**: Ubuntu 22.04 base image with Rust, Clang 17, `openssl`, `libcrypto`, `pcap`, and offline packet simulation harnesses in `/app`.
- **`resourceEstimate`**:
  - `cpuMillis`: 8000
  - `memoryMb`: 8192
  - `storageMb`: 10240
  - `gpuCount`: 0
  - `agentTimeoutSec`: 28800 (8 hours)
  - `verifierTimeoutSec`: 1200 (20 mins)
- **`networkRequirements`**: `none`

### Oracle & Verification
- **`oracleStrategy`**: Reference implementation managing ring-buffer based zero-copy frame views, a sliding window packet number filter, and a state machine conforming to RFC 9000/9002.
- **`verificationStrategy`**:
  - **Public Tests**: Frame parsing unit tests, basic bidirectional stream loopback, and loss-free payload transfer.
  - **Hidden Tests**:
    1. Replay of 1,000+ pathological packet traces (jitter, reordering, duplicate frames, out-of-order 0-RTT/1-RTT transitions).
    2. Simulated high packet loss (20%) network channels measuring flow control efficiency and exact byte-for-byte stream reconstruction.
    3. Fuzzing injection testing malformed frames, integer overflows, and frame length spoofing.

### Scoring & Anti-Exploits
- **`binarySuccessCondition`**: Correctly processes all packet capture streams, accurately handles 100% of out-of-order / packet loss retransmissions, and passes the RFC 9000 conformance test matrix.
- **`partialScoreStrategy`**:
  - 25%: RFC 9000 header & frame parsing conformance.
  - 25%: Lossless stream reassembly with out-of-order payloads.
  - 25%: Flow control & ACK congestion management.
  - 25%: Resilience against malformed/fuzzed input packets without panics or leaks.
- **`anticipatedExploits`**:
  - *Exploit*: Buffering entire streams in unbounded dynamic memory to bypass flow control limits.
  - *Mitigation*: Memory allocator hooks limit heap usage to fixed bounds; verifier enforces stream backpressure tests.

---

## Task 3: SIMD-Accelerated Distributed Vector Index (HNSW + PQ) with Quantization

### Draft Metadata

- **`title`**: AVX-512 / NEON Accelerated HNSW Vector Graph with Product Quantization
- **`workingSlug`**: `hnsw-pq-simd-vector-engine`
- **`collectionFamily`**: `Algorithmic optimization`
- **`taskFamily`**: `performance`
- **`verifierFamily`**: `optimization`
- **`expertTimeEstimateHours`**: 14

### Problem & Scope
- **`objective`**: Build a high-performance Hierarchical Navigable Small World (HNSW) vector search index accelerated with Product Quantization (PQ) and hand-optimized SIMD intrinsics (AVX-512 / AVX2 / NEON fallback) in C++ or Rust. The engine must support incremental vector insertion, dynamic heuristic neighbor pruning ($M$, $efConstruction$, $efSearch$), 8-bit asymmetric distance computation (ADC) tables, and concurrent multi-threaded k-NN queries achieving >95% recall@10 on 1M 768-dimensional vectors with sub-millisecond latency.
- **`motivation`**: Vector search underpins modern generative AI retrieval (RAG) and embedding databases. Achieving the Pareto frontier of recall vs. latency requires deep hardware-level understanding of memory cache hierarchies, prefetching, and vector SIMD instructions.

### Difficulty & Reasoning
- **`difficultyExplanation`**:
  1. **Asymmetric Distance Computation (ADC) SIMD Kernels**: Computing distances between unquantized query vectors and PQ-quantized centroid lookups using SIMD gather/shuffle intrinsics without cache line stalls.
  2. **Concurrent HNSW Graph Construction**: Maintaining graph connectivity and bidirectionally updating entry points under concurrent lock-free vector insertions.
  3. **Recall / Latency Trade-Off**: Sub-optimal heuristic neighbor selection causes the search to get stuck in local graph minima unless diverse edge selection heuristics are strictly implemented.

### Environment & Compute Budget
- **`environmentSummary`**: Ubuntu 24.04 with `gcc-14`, `clang-18`, AVX2/AVX-512 emulation tooling, pre-downloaded 768-dim embedding datasets (100k and 1M vectors), and benchmarking suites in `/app`.
- **`resourceEstimate`**:
  - `cpuMillis`: 8000
  - `memoryMb`: 32768 (32 GB)
  - `storageMb`: 20480 (20 GB)
  - `gpuCount`: 0
  - `agentTimeoutSec`: 28800 (8 hours)
  - `verifierTimeoutSec`: 1800 (30 mins)
- **`networkRequirements`**: `none`

### Oracle & Verification
- **`oracleStrategy`**: Reference C++20 implementation using AVX2/AVX-512 integer lookup tables (`_mm256_shuffle_epi8` / `_mm512_permutexvar_epi8`) for ADC distance estimation and lock-free concurrent HNSW graph updates.
- **`verificationStrategy`**:
  - **Public Tests**: Small-scale 10k vector recall and query correctness verification.
  - **Hidden Tests**:
    1. Full 1M vector dataset evaluation: Verifier measures query throughput (QPS), p99 latency, and recall@10 against a sealed ground-truth brute-force index.
    2. Concurrent insertion + query stress test testing thread safety and index consistency.
    3. Performance gate: Must achieve ≥ 95% Recall@10 at ≥ 15,000 QPS on 8 vCPUs.

### Scoring & Anti-Exploits
- **`binarySuccessCondition`**: Recall@10 ≥ 0.95 and Query Throughput ≥ 15,000 QPS on the 1M vector benchmark dataset with zero concurrency crashes.
- **`partialScoreStrategy`**: Continuous metric based on $Score = \max(0, \frac{\text{Recall} - 0.70}{0.25}) \times \min(1.0, \frac{\text{QPS}}{15000})$.
- **`anticipatedExploits`**:
  - *Exploit*: Brute-force linear scan using all CPU cores.
  - *Mitigation*: QPS threshold (15,000 QPS) is mathematically impossible with brute-force linear search over 768-dim 1M vectors on 8 CPUs.

---

## Task 4: Distributed Raft Consensus Engine with Pre-Vote, Joint Consensus & Snapshotting

### Draft Metadata

- **`title`**: Production-Grade Distributed Raft Consensus Engine with Pre-Vote & Dynamic Reconfiguration
- **`workingSlug`**: `raft-distributed-consensus-engine`
- **`collectionFamily`**: `Product clone`
- **`taskFamily`**: `systems_integration`
- **`verifierFamily`**: `programmatic`
- **`expertTimeEstimateHours`**: 18

### Problem & Scope
- **`objective`**: Build a production-grade distributed consensus cluster implementing the full Raft protocol in Rust or Go/C++20, including Leader Election, Log Replication, Log Compaction & Snapshotting (`InstallSnapshot` RPC), Pre-Vote extension (preventing disruptive re-elections during network partitions), Linearizable Read index queries (lease read / ReadIndex without log writes), and Joint Consensus dynamic membership reconfiguration ($C_{\text{old}} \to C_{\text{old,new}} \to C_{\text{new}}$).
- **`motivation`**: Distributed state machines (etcd, CockroachDB, TiKV) rely on Raft for data consistency. Subtleties in uncommitted log overwrites, split-brain recovery after asymmetric network partitions, and joint consensus transitions routinely confuse state-of-the-art coding agents.

### Difficulty & Reasoning
- **`difficultyExplanation`**:
  1. **Asymmetric Network Partitions & Pre-Vote**: Handling non-transitive or unidirectional network cuts where a partitioned node repeatedly increments terms and disrupts healthy leaders unless Pre-Vote phases filter unviable candidates.
  2. **Joint Consensus Membership Changes**: Safely executing multi-node configuration changes without split-brain by requiring separate majorities from both configuration sets simultaneously during transitions.
  3. **Snapshotting vs. In-Flight Log Replication**: Coordinating log truncation, snapshot transfer streams, and catch-up log replay for lagging or restarted follower nodes without stalling the leader or corrupting log indices.

### Environment & Compute Budget
- **`environmentSummary`**: Debian 12 base container with Rust 1.78, Go 1.22, Clang 18, CMake, and a deterministic network chaos testbed (simulating latency, packet drops, partitions, and clock drift) in `/app`.
- **`resourceEstimate`**:
  - `cpuMillis`: 8000
  - `memoryMb`: 16384 (16 GB)
  - `storageMb`: 10240 (10 GB)
  - `gpuCount`: 0
  - `agentTimeoutSec`: 28800 (8 hours)
  - `verifierTimeoutSec`: 1800 (30 mins)
- **`networkRequirements`**: `none` (offline harness simulates inter-node RPCs in-memory / loopback)

### Oracle & Verification
- **`oracleStrategy`**: Reference implementation featuring an asynchronous event-loop Raft state machine, non-blocking storage abstraction, atomic snapshot installer, and Jepsen-validated linearizable client state machine.
- **`verificationStrategy`**:
  - **Public Tests**: Basic 3-node election, log replication, single node crash and recovery.
  - **Hidden Tests**:
    1. Jepsen-style Maelstrom chaos testing (5-node cluster subjected to random dynamic partitions, leader isolation, packet drops, and asymmetric routing).
    2. Dynamic membership test: Adding 2 nodes and removing 1 node concurrently while serving 10,000 continuous client writes.
    3. Linearizability validation: Verifying zero stale reads under `ReadIndex` queries during leader step-down events.

### Scoring & Anti-Exploits
- **`binarySuccessCondition`**: Passes 100% of network partition chaos tests, guarantees linearizability across all client read/write histories, and completes membership migrations without downtime or split-brain.
- **`partialScoreStrategy`**:
  - 25%: Basic leader election & persistent log replication.
  - 25%: Snapshot transfer (`InstallSnapshot`) and log compaction.
  - 25%: Pre-Vote resilience under asymmetric partitions.
  - 25%: Joint consensus membership transitions & linearizable reads.
- **`anticipatedExploits`**:
  - *Exploit*: Using single-node centralized lock server or wrapping embedded SQLite.
  - *Mitigation*: Verifier injects network faults between virtual cluster nodes and inspects peer RPC message traces directly.

---

## Task 5: Paged FlashAttention-2 Triton / CUDA Kernel with Backward Gradient Pass

### Draft Metadata

- **`title`**: Paged FlashAttention-2 Custom Kernel with Backward Pass and Variable-Length Packing
- **`workingSlug`**: `flash-attention-paged-triton`
- **`collectionFamily`**: `ML engineering`
- **`taskFamily`**: `performance`
- **`verifierFamily`**: `optimization`
- **`expertTimeEstimateHours`**: 16

### Problem & Scope
- **`objective`**: Implement a custom FlashAttention-2 GPU kernel in OpenAI Triton or CUDA with complete Forward and Backward gradient passes, supporting Paged KV-Cache (non-contiguous memory blocks for vLLM-style serving), variable-length sequence packing (`cu_seqlens`), causal masking, and rotary position embedding (RoPE) fusion in FP16 / BF16 precision. The kernel must match or exceed PyTorch `F.scaled_dot_product_attention` numerical accuracy while achieving >70% theoretical FP16 tensor core TFLOPS.
- **`motivation`**: LLM training and inference latency is dominated by attention memory bandwidth. Custom kernel development requires fine-grained control of GPU shared memory (SRAM), warp tile orchestration, online softmax rescaling, and memory coalescing.

### Difficulty & Reasoning
- **`difficultyExplanation`**:
  1. **Online Softmax Rescaling & Numerics**: Maintaining running maximums and normalization factors ($m_i, l_i$) across tiles in SRAM to prevent FP16 overflow/underflow without materializing the $O(N^2)$ attention matrix.
  2. **Backward Gradient Computation ($dQ, dK, dV$)**: Recomputing intermediate attention weights on the fly during the backward pass using SRAM tiles, and computing softmax derivatives ($dS = P \odot (dO \cdot V^T - D)$) with exact numerical stability.
  3. **Paged KV-Cache Memory Indirection**: Resolving non-contiguous physical page table blocks for Key/Value tensors in SRAM without breaking warp coalescing or vector load strides.

### Environment & Compute Budget
- **`environmentSummary`**: Ubuntu 22.04 with PyTorch 2.3, CUDA 12.4 toolkit, Triton 2.3.0, and standard LLM evaluation benchmark harness in `/app`.
- **`resourceEstimate`**:
  - `cpuMillis`: 8000
  - `memoryMb`: 32768 (32 GB)
  - `storageMb`: 20480 (20 GB)
  - `gpuCount`: 0
  - `agentTimeoutSec`: 28800 (8 hours)
  - `verifierTimeoutSec`: 1800 (30 mins)
- **`networkRequirements`**: `none`

### Oracle & Verification
- **`oracleStrategy`**: Reference Triton/CUDA kernel implementing 2D/3D grid block scheduling, shared memory double buffering, fused causal masking, and atomic reduction for backward gradient accumulation.
- **`verificationStrategy`**:
  - **Public Tests**: Forward pass correctness check against `torch.nn.functional.scaled_dot_product_attention` on fixed batch size.
  - **Hidden Tests**:
    1. Gradient check: `torch.autograd.gradcheck` evaluating analytical gradients ($dQ, dK, dV$) against numerical finite-difference baselines ($L_\infty < 10^{-3}$).
    2. Paged KV-cache inference benchmark across arbitrary context lengths (1k to 32k tokens) and sequence batching.
    3. Throughput & TFLOPS benchmark: Measuring execution latency and tensor core utilization floor (>65% peak TFLOPS).

### Scoring & Anti-Exploits
- **`binarySuccessCondition`**: Passes forward/backward gradient checks with absolute error $\le 10^{-3}$, supports paged KV-cache indirection, and achieves $\ge 65\%$ peak compute efficiency.
- **`partialScoreStrategy`**: Continuous metric combining accuracy ($\Delta \le 10^{-3}$) and throughput speedup ($S = \text{TFLOPS} / \text{Baseline}$).
- **`anticipatedExploits`**:
  - *Exploit*: Calling existing PyTorch `sdpa` directly.
  - *Mitigation*: Verifier injects non-contiguous paged memory layouts unsupported by stock PyTorch and profiles custom kernel launches via `nvprof` / CUDA runtime events.

---

## Task 6: Zero-Copy WebAssembly (WASM) Core Engine with Tier-1 Baseline JIT Compiler

### Draft Metadata

- **`title`**: Zero-Copy WASM Core Engine with Tier-1 JIT Compiler and Spec Conformance
- **`workingSlug`**: `wasm-jit-core-runtime`
- **`collectionFamily`**: `Library clone`
- **`taskFamily`**: `feature_development`
- **`verifierFamily`**: `programmatic`
- **`expertTimeEstimateHours`**: 18

### Problem & Scope
- **`objective`**: Implement a standalone WebAssembly (WASM MVP + SIMD / Multi-Value) runtime engine in C++20 or Rust, consisting of a zero-copy bytecode validator and a Tier-1 baseline JIT compiler generating native x86-64 machine code directly into executable memory pages (`mmap` `PROT_EXEC`). The runtime must execute WASM control flow (`block`, `loop`, `if`, `br_table`), stack value operations, linear memory bounds checking, host imports/exports, and pass 100% of the official WebAssembly Core Specification test suite.
- **`motivation`**: WASM is the universal runtime for serverless edge computing, plugin sandboxes, and browser execution. Building a lightweight JIT runtime tests low-level ABI calling conventions, machine code generation, register allocation, and bytecode validation.

### Difficulty & Reasoning
- **`difficultyExplanation`**:
  1. **Control Flow Machine Code Emission**: Lowering structured WASM control blocks (`block`, `loop`, `br_if`, `br_table`) into native jump offsets, managing stack unwind depths and branch targets in single-pass machine code generation.
  2. **Fast Linear Memory Bounds Checking**: Implementing efficient software or signal-handler/guard-page based memory bounds enforcement for `i32.load`/`i64.store` without massive runtime overhead.
  3. **Multi-Value & System V AMD64 ABI Integration**: Interfacing WASM function calls and host imports conforming to System V AMD64 ABI register assignments (`rdi`, `rsi`, `rdx`, `rcx`, `r8`, `r9`, `xmm0-7`) with zero intermediate allocation.

### Environment & Compute Budget
- **`environmentSummary`**: Debian 12 with `gcc-13`, `clang-18`, `cmake`, `ninja-build`, `wabt` (WebAssembly Binary Toolkit), and the official W3C WASM Core Spec test suite pre-installed in `/app`.
- **`resourceEstimate`**:
  - `cpuMillis`: 8000
  - `memoryMb`: 16384 (16 GB)
  - `storageMb`: 10240 (10 GB)
  - `gpuCount`: 0
  - `agentTimeoutSec`: 28800 (8 hours)
  - `verifierTimeoutSec`: 1800 (30 mins)
- **`networkRequirements`**: `none`

### Oracle & Verification
- **`oracleStrategy`**: Reference C++20 implementation providing a streaming bytecode validator, a single-pass x86-64 assembler emitting native opcodes directly to executable memory, and a sandboxed host interface.
- **`verificationStrategy`**:
  - **Public Tests**: Basic arithmetic, recursion (`fibonacci`), and function import/export tests.
  - **Hidden Tests**:
    1. Official W3C WebAssembly 1.0/2.0 Core Specification Test Suite (thousands of spec assertion `.wast` test files).
    2. Fault and bounds safety testing: Out-of-bounds memory access, divide-by-zero, and stack overflow traps correctly intercepted via signal handlers (`SIGSEGV`/`SIGFPE`) without crashing the host process.
    3. JIT execution speed benchmark comparing native JIT execution vs. interpreted execution (>10x speedup).

### Scoring & Anti-Exploits
- **`binarySuccessCondition`**: Passes $\ge 98\%$ of the official W3C Core Spec test suite, properly catches all memory traps, and compiles to native x86-64 machine code.
- **`partialScoreStrategy`**:
  - 30%: Bytecode validation and basic opcodes (`i32`/`i64` arithmetic, memory).
  - 30%: Structured control flow (`block`, `loop`, `br_table`, function calls).
  - 20%: W3C Core Spec test suite conformance.
  - 20%: Native JIT execution speedup and zero memory leaks under ASan.
- **`anticipatedExploits`**:
  - *Exploit*: Linking against `wasmtime`, `wasmer`, or `v8`.
  - *Mitigation*: Static symbol verification and disassembling generated code pages in memory to confirm custom single-pass machine code generation.

---

## Task 7: Stateful eBPF/XDP High-Speed Firewall with Conntrack & SYN-Flood Defense

### Draft Metadata

- **`title`**: Stateful eBPF/XDP High-Speed Packet Filter with TCP Conntrack and Rate Limiting
- **`workingSlug`**: `ebpf-xdp-stateful-firewall`
- **`collectionFamily`**: `Product clone`
- **`taskFamily`**: `systems_integration`
- **`verifierFamily`**: `programmatic`
- **`expertTimeEstimateHours`**: 16

### Problem & Scope
- **`objective`**: Implement a high-performance stateful network firewall engine in C/C++ using Linux eBPF and XDP (eXpress Data Path). The engine must handle packet parsing (Ethernet, IPv4/IPv6, TCP, UDP, ICMP), bidirectional TCP connection state tracking (conntrack table managing `SYN_SENT`, `ESTABLISHED`, `FIN_WAIT`, `CLOSED` with timeout expiration and sequence validation), dynamic token-bucket rate limiting per CIDR subnet, and cryptographic SYN cookies to mitigate SYN flood attacks at 10M+ packets per second.
- **`motivation`**: Cloud-native perimeter defense and software-defined networks increasingly run packet filtering in the Linux kernel via eBPF/XDP. Writing production eBPF programs requires passing strict kernel BPF verifier constraints (bounded loops, memory access safety, atomic map updates) while preserving multi-million packet-per-second line rate performance.

### Difficulty & Reasoning
- **`difficultyExplanation`**:
  1. **Kernel BPF Verifier Constraints**: Meeting strict kernel verifier safety rules (bounded loops, pointer arithmetic bounds, packet re-validation after helper calls) without falling back to slow user-space processing.
  2. **Concurrent Conntrack Map Synchronization**: Managing atomic LRU hash maps for millions of active flows under concurrent multi-core packet arrival without race conditions or memory leaks during TCP state transitions.
  3. **SYN-Cookie Cryptographic Validation**: Generating and validating stateless TCP sequence numbers containing encoded timestamp and MSS options in XDP before committing state to the connection tracking table.

### Environment & Compute Budget
- **`environmentSummary`**: Ubuntu 24.04 with Linux kernel 6.8 header packages, `clang-18`, `llvm-18`, `libbpf-dev`, `iproute2`, and a virtual network namespace packet generator testbed in `/app`.
- **`resourceEstimate`**:
  - `cpuMillis`: 8000
  - `memoryMb`: 16384 (16 GB)
  - `storageMb`: 10240 (10 GB)
  - `gpuCount`: 0
  - `agentTimeoutSec`: 28800 (8 hours)
  - `verifierTimeoutSec`: 1800 (30 mins)
- **`networkRequirements`**: `none` (traffic is generated and evaluated within offline Linux network namespaces)

### Oracle & Verification
- **`oracleStrategy`**: Reference implementation featuring an XDP driver-mode bytecode hook, BPF LRU hash maps for flow tracking, user-space management daemon via `libbpf`, and atomic token-bucket rate limiters.
- **`verificationStrategy`**:
  - **Public Tests**: Basic packet parsing, UDP pass-through, and static 5-tuple ACL filtering.
  - **Hidden Tests**:
    1. Stateful TCP handshakes & teardown sequences across 100k simulated client flows verifying out-of-order sequence rejection and expired flow eviction.
    2. SYN Flood attack simulation (1M syn packets/sec) verifying stateless SYN-cookie generation and legitimate client connection establishment under attack.
    3. Token-bucket rate limiter verification: Measuring precise bandwidth/PPS throttling per source CIDR block.

### Scoring & Anti-Exploits
- **`binarySuccessCondition`**: Correctly processes 100% of stateful TCP transitions, passes BPF kernel verifier with 0 rejections, and successfully mitigates SYN flood simulation without dropping legitimate established traffic.
- **`partialScoreStrategy`**:
  - 25%: Static 5-tuple packet parsing and basic ACL rules.
  - 25%: TCP 4-way handshake conntrack state machine & timer GC.
  - 25%: Token-bucket rate limiting per IP CIDR subnet.
  - 25%: SYN-cookie cryptographic generation and high-throughput flood mitigation.
- **`anticipatedExploits`**:
  - *Exploit*: Using iptables / nftables CLI wrappers in user space.
  - *Mitigation*: Verifier inspects attached XDP BPF program file descriptors (`bpftool prog show`) and disables host iptables kernel modules.

---

## Task 8: Zero-Copy SIMD Columnar Parquet & Arrow Stream Decoder

### Draft Metadata

- **`title`**: Zero-Copy SIMD Columnar Parquet Decoder with Predicate Pushdown
- **`workingSlug`**: `simd-parquet-columnar-decoder`
- **`collectionFamily`**: `Library clone`
- **`taskFamily`**: `performance`
- **`verifierFamily`**: `programmatic`
- **`expertTimeEstimateHours`**: 16

### Problem & Scope
- **`objective`**: Implement a high-performance, zero-copy Apache Parquet / Arrow columnar format decoder in C++20 or Rust. The engine must support Parquet metadata parsing (Thrift schema headers, RowGroups, ColumnChunks), dictionary decoding, Run-Length Encoding (RLE), bit-packing with AVX2 SIMD acceleration, Snappy/ZSTD page decompression, and vectorized predicate pushdown evaluation (evaluating boolean filters like `col > 100 AND col < 500` directly on encoded bit-packed streams to skip decompression of non-matching RowGroups).
- **`motivation`**: Analytical query engines (DuckDB, ClickHouse, Apache Arrow) rely on vectorized columnar readers for ultra-high scan throughput. Implementing a clean, zero-copy Parquet decoder from scratch tests bitwise manipulation, SIMD unpack algorithms, memory-mapped I/O, and columnar filter evaluation.

### Difficulty & Reasoning
- **`difficultyExplanation`**:
  1. **SIMD Bit-Unpacking Kernels**: Implementing AVX2/NEON parallel bit-unpacking routines (e.g. 1-bit to 32-bit width unpacking) that unpack 32 integers per SIMD register without scalar branch loops.
  2. **Definition & Repetition Level Decoding**: Decoding variable-depth nested schemas using hybrid RLE/Bit-Packed level encoders to correctly reconstruct null bitmaps and nested list offsets.
  3. **Zero-Copy Predicate Pushdown**: Evaluating min/max page statistics and dictionary indices directly in compressed page buffers to prune whole RowGroups without materializing columnar arrays.

### Environment & Compute Budget
- **`environmentSummary`**: Debian 12 with `gcc-13`, `clang-18`, `cmake`, `ninja-build`, `libsnappy-dev`, `libzstd-dev`, and pre-generated multi-gigabyte Parquet test datasets in `/app`.
- **`resourceEstimate`**:
  - `cpuMillis`: 8000
  - `memoryMb`: 16384 (16 GB)
  - `storageMb`: 10240 (10 GB)
  - `gpuCount`: 0
  - `agentTimeoutSec`: 28800 (8 hours)
  - `verifierTimeoutSec`: 1800 (30 mins)
- **`networkRequirements`**: `none`

### Oracle & Verification
- **`oracleStrategy`**: Reference C++20 decoder implementing fast bit-unpacking SIMD tables, compact Thrift schema parser, and an Arrow RecordBatch zero-copy exporter.
- **`verificationStrategy`**:
  - **Public Tests**: Small single-column Parquet file reading and schema validation.
  - **Hidden Tests**:
    1. Reading 100+ complex Parquet files with mixed encodings (Plain, Dictionary, RLE, Bit-Packed, Snappy, ZSTD, nested structs).
    2. Correctness validation comparing extracted Arrow record batches against Apache Arrow official reference output down to exact bitwise IEEE 754 precision.
    3. Throughput & Predicate Pushdown benchmark: Must achieve >= 2.5 GB/s scan throughput on multi-column datasets with filter pushdown.

### Scoring & Anti-Exploits
- **`binarySuccessCondition`**: 100% exact match against Apache Arrow reference decoding on all test datasets, predicate pushdown successfully prunes pages, and scan throughput >= 2.0 GB/s.
- **`partialScoreStrategy`**:
  - 25%: Basic Parquet Thrift header parsing & plain encoding decoding.
  - 25%: Dictionary & RLE / Bit-Packed SIMD decoding.
  - 25%: Snappy & ZSTD compressed page decompression.
  - 25%: Predicate pushdown filter evaluation & throughput speedup.
- **`anticipatedExploits`**:
  - *Exploit*: Linking against `libparquet` or `arrow-cpp`.
  - *Mitigation*: Static symbol verification, binary header inspection, and validating custom SIMD kernel execution traces.

---

## Task 9: AVX-512 / AVX2 Accelerated 32-Qubit Quantum Circuit State-Vector Simulator

### Draft Metadata

- **`title`**: AVX-512 / AVX2 Accelerated 32-Qubit Quantum State-Vector Simulator with Gate Fusion
- **`workingSlug`**: `simd-quantum-circuit-simulator`
- **`collectionFamily`**: `Algorithmic optimization`
- **`taskFamily`**: `performance`
- **`verifierFamily`**: `optimization`
- **`expertTimeEstimateHours`**: 16

### Problem & Scope
- **`objective`**: Implement an ultra-fast, multi-threaded 32-qubit state-vector quantum circuit simulator in C++20 accelerated by hand-optimized AVX2 / AVX-512 complex arithmetic intrinsics. The engine must support arbitrary quantum gates (Hadamard, Pauli-X/Y/Z, Phase/T gates, parameterized Rotation gates $R_x, R_y, R_z$, and two-qubit entangling gates CNOT, CZ, SWAP), static unitary gate fusion (merging sequences of single-qubit and two-qubit gates into dense $4 \times 4$ or $8 \times 8$ unitary matrix kernels), cache-blocking state-vector strides, and parallel multi-qubit measurement sampling.
- **`motivation`**: Quantum circuit simulation is heavily compute- and memory-bandwidth-bound ($2^{32}$ complex amplitudes require 64 GB of memory in complex64). Pushing simulation performance requires advanced SIMD complex multiplication, cache-locality optimization across non-contiguous index strides, and algebraic gate fusion.

### Difficulty & Reasoning
- **`difficultyExplanation`**:
  1. **SIMD Complex Arithmetic on Strided Amplitudes**: Applying quantum gates across non-contiguous qubit target indices requires fast SIMD permutations (`_mm256_permute2f128_ps`, `_mm256_shuffle_ps`) to avoid strided memory access bottlenecks.
  2. **Algebraic Gate Fusion**: Analyzing circuit DAGs to automatically coalesce consecutive 1-qubit and 2-qubit operations acting on overlapping target qubits into fused multi-qubit unitary matrices to minimize memory bandwidth passes.
  3. **Multi-Threaded Cache-Blocking**: Partitioning the $2^N$ state vector into L1/L2 cache-sized sub-blocks so multiple gates can be applied in SRAM without round-tripping to main RAM.

### Environment & Compute Budget
- **`environmentSummary`**: Debian 12 with `gcc-13`, `clang-18`, `cmake`, `ninja-build`, and Python 3.10 verification harness (Qiskit / Cirq) pre-installed in `/app`.
- **`resourceEstimate`**:
  - `cpuMillis`: 8000
  - `memoryMb`: 65536 (64 GB)
  - `storageMb`: 20480 (20 GB)
  - `gpuCount`: 0
  - `agentTimeoutSec`: 28800 (8 hours)
  - `verifierTimeoutSec`: 1800 (30 mins)
- **`networkRequirements`**: `none`

### Oracle & Verification
- **`oracleStrategy`**: Reference C++20 implementation providing AVX2/AVX-512 fused $2 \times 2$ and $4 \times 4$ unitary kernels, OpenMP thread partitioning, and an automated circuit DAG fusion optimizer.
- **`verificationStrategy`**:
  - **Public Tests**: Single-qubit superposition (Bell state), GHZ state generation, and quantum Fourier transform (QFT) on 8 qubits.
  - **Hidden Tests**:
    1. Quantum Random Circuit Sampling (RCS) benchmarks on 24 to 30 qubits verifying final state-vector fidelity ($F > 0.999999$) against exact reference mathematical state vectors.
    2. Gate fusion correctness testing: Verifying that fused matrix kernels produce exact mathematical equivalences across arbitrary rotation angles.
    3. Performance benchmark: Must achieve >= 10x speedup over standard scalar state-vector matrix-vector multiplication baselines.

### Scoring & Anti-Exploits
- **`binarySuccessCondition`**: State vector fidelity $F \ge 0.99999$ across all test circuits, zero race conditions, and >= 8x execution speedup over baseline un-fused simulator.
- **`partialScoreStrategy`**: Continuous metric combining numerical fidelity ($F \ge 0.999$) and execution throughput speedup ($S = \text{Time}_{\text{baseline}} / \text{Time}_{\text{engine}}$).
- **`anticipatedExploits`**:
  - *Exploit*: Storing precomputed final state vectors for known benchmark circuits.
  - *Mitigation*: Verifier generates dynamic random quantum circuits with randomized continuous rotation parameters at evaluation time.

