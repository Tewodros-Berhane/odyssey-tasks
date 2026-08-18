# Lock-Free Concurrent B+ Tree Storage Engine with Direct I/O and ARIES Recovery

## Objective
Implement a production-grade, concurrent database storage engine in C++20. The system must feature a custom Buffer Pool Manager bypassing the OS page cache via `O_DIRECT` (aligned to 4KB sectors), a lock-free B+ Tree index utilizing Hazard Pointers and optimistic latch coupling, and an ARIES-style Write-Ahead Log (WAL) capable of recovering database state (Analysis, Redo, Undo) after crash failures.

## Guidelines
1. **Buffer Pool**: Implement LRU-K cache eviction and use `posix_memalign` for 4KB `O_DIRECT` I/O.
2. **B+ Tree**: Implement lock-free, concurrent CRUD operations using Hazard Pointers to prevent ABA memory corruption.
3. **WAL / ARIES**: Implement Log Sequence Numbers (LSNs) and a 3-pass crash recovery mechanism.
4. **Starter Code**: The starter environment in `environment/src/` provides empty skeletons. Implement the engines in these files.
5. **No Global Mutexes**: Ensure thread safety under high contention without `std::mutex` bottlenecks.

Your solution will be tested against AddressSanitizer and ThreadSanitizer.
