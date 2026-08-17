# In-Memory MVCC B-Tree Storage Engine with WAL and ARIES Recovery

## Overview
You are tasked with completing a production-grade, concurrent Multi-Version Concurrency Control (MVCC) B+Tree storage engine in C++20 with Write-Ahead Logging (WAL) and ARIES crash recovery.

The codebase is located in `/app`.

## Architecture & Requirements

### 1. B+Tree Storage & Node Concurrency
- Implement a thread-safe B+Tree index supporting point lookups, insertions, updates, deletions, and forward/backward range scans.
- Support concurrency via latch-crabbing (or optimistic read lock validation / B-link style right-sibling pointers).
- Node splits and merges must execute cleanly under concurrent multi-threaded reader/writer contention without deadlocks or corrupted pointers.

### 2. Multi-Version Concurrency Control (MVCC) & Snapshot Isolation
- Implement Snapshot Isolation (SI):
  - Every transaction is assigned a `read_ts` at start and a monotonic `commit_ts` at commit.
  - Transactions must only see tuple versions committed with `commit_ts <= read_ts`.
  - Uncommitted modifications by the current transaction are visible to itself.
  - First-committer-wins / write-write conflict detection: abort any transaction attempting to update/delete a key concurrently modified by another active transaction.
- Version Chains & Garbage Collection:
  - Maintain historical version chains for updated/deleted keys.
  - Non-blocking garbage collection (vacuuming) must prune version records older than the oldest active transaction's `read_ts`.

### 3. Write-Ahead Logging (WAL) & Group Commit
- Implement an append-only WAL manager:
  - Log records must follow a monotonic Log Sequence Number (`LSN`) progression.
  - Log record types: `BEGIN`, `UPDATE`, `INSERT`, `DELETE`, `COMMIT`, `ABORT`, `CLR` (Compensation Log Record), and `CHECKPOINT`.
  - Group commit: Flush WAL buffers periodically or when a threshold of transactions request sync, without issuing blocking `fsync` per transaction.
  - WAL invariant: A page modified with an update associated with `LSN` must never be written to persistent store before the WAL up to that `LSN` has been flushed to disk (`pageLSN <= flushedLSN`).

### 4. ARIES Crash Recovery
The engine must support deterministic recovery following an abrupt crash or `SIGKILL`:
- **Phase 1 (Analysis)**:
  - Scan the WAL forward starting from the last checkpoint record.
  - Reconstruct the Transaction Table (active transactions at crash time) and Dirty Page Table (`DPT`, recording `recLSN` for dirty pages).
- **Phase 2 (Redo)**:
  - Scan forward from the smallest `recLSN` across all pages in the `DPT`.
  - Replay all logged actions (including updates from aborted transactions and CLRs) to bring database state to the exact moment of failure ("repeating history").
- **Phase 3 (Undo)**:
  - Scan backward through the WAL to undo changes made by transactions that were active (uncommitted) at crash time.
  - For each undone operation, log a Compensation Log Record (`CLR`) with an `undoNextLSN` pointer to ensure recovery can survive crashes during restart.

## Build and Test Instructions
The project uses CMake and C++20:
```bash
cd /app
mkdir -p build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build . --parallel $(nproc)
ctest --output-on-failure
```

The verifier executes `tests/test.sh`, which runs functional tests, concurrency stress suites (64 threads), crash simulation, and Address/Thread Sanitizers.
