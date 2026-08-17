# Production-Grade Distributed Raft Consensus Engine with Pre-Vote & Dynamic Reconfiguration

## Overview
Your objective is to implement a production-grade distributed consensus engine in C++20 conforming to the Raft protocol specifications in `/app`.

## Architecture & Requirements

### 1. Leader Election & Heartbeats
- Maintain role state: `Follower`, `Candidate`, `PreCandidate`, `Leader`.
- Randomized election timers (e.g. 150ms–300ms) with background tick management.
- State invariants: `currentTerm`, `votedFor`, `log[]`, `commitIndex`, `lastApplied`.
- Step-down rules: If any message contains `term > currentTerm`, update term and revert to `Follower`.

### 2. Pre-Vote Extension
- Implement Pre-Vote phase before incrementing `currentTerm` during candidate election timeouts.
- Node sends `RequestPreVote` with proposed `currentTerm + 1`.
- Followers only grant pre-votes if:
  1. The candidate's log is at least as up-to-date as the follower's log.
  2. The follower has not heard from an active leader within the election timeout window.
- Only advance to `Candidate` and increment `currentTerm` if a majority grants Pre-Votes.

### 3. Log Replication & Safety
- Leader replicates `AppendEntries` RPCs to followers.
- Handle uncommitted log conflicts: find mismatching index, truncate conflicting follower entries, and append new entries.
- Monotonic commit index progression: Leader commits entry once replicated to a majority quorum for the current term.

### 4. Log Compaction & Snapshotting (`InstallSnapshot`)
- Implement log truncation and periodic checkpointing to state machine snapshots.
- If a follower's `nextIndex <= lastIncludedIndex`, leader sends `InstallSnapshot` RPC instead of `AppendEntries`.
- Atomic installation of snapshot into follower state machine without race conditions.

### 5. Joint Consensus Dynamic Membership Changes
- Reconfigure cluster membership dynamically via 2-phase Joint Consensus:
  - Phase 1: Propose and commit $C_{\text{old,new}}$ configuration entry. Quorum decisions require majorities from both $C_{\text{old}}$ and $C_{\text{new}}$ independently.
  - Phase 2: Propose and commit final $C_{\text{new}}$ configuration entry once $C_{\text{old,new}}$ is committed.

### 6. Linearizable Reads (ReadIndex)
- Process client read queries without writing dummy entries to the log:
  - Record `readIndex = commitIndex`.
  - Confirm leadership status via a heartbeat round with a majority quorum.
  - Wait until `lastApplied >= readIndex`, then execute read against the local state machine.

## Build and Test Instructions
The project uses CMake and C++20:
```bash
cd /app
mkdir -p build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build . --parallel $(nproc)
ctest --output-on-failure
```

The verifier executes `tests/test.sh`, evaluating election stability, network partition resilience, snapshot installation, and dynamic reconfiguration.
