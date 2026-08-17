# Lock-Free Thread-Caching Memory Allocator with Hazard Pointers and Size-Class Arenas

## Overview
Your objective is to implement a high-performance, lock-free thread-caching dynamic memory allocator in C++20 with Hazard Pointers and size-class arenas in `/app`.

## Architecture & Requirements

### 1. Segregated Size-Classes & Slab Geometry
- Support power-of-two and intermediate size classes:
  - Small bins: 8, 16, 32, 48, 64, 96, 128, 192, 256, 384, 512, 768, 1024 bytes.
  - Medium/Large bins: 2 KiB up to 64 KiB.
  - Huge pages: Direct `mmap` allocation for requests > 64 KiB.
- Align all returned pointers to at least `alignof(std::max_align_t)` (16 bytes on x86-64).

### 2. Thread-Local Caching & Lock-Free Batch Transfers
- Maintain a thread-local cache (`ThreadCache`) per thread containing freelists for small size classes.
- Fast path: Allocate/deallocate directly from thread-local freelist without atomic operations.
- Slow path / Refill: When a local bin is exhausted, fetch a batch of free objects from the central slab repository (`CentralArena`) using lock-free CAS (`compare_exchange_weak`).
- Flush: When local bin capacity exceeds a threshold, return a batch to the central arena.

### 3. Hazard Pointers & Safe Deferred Memory Reclamation
- Implement a lock-free Hazard Pointer table to prevent ABA corruption and use-after-free during concurrent remote frees (where thread A frees memory originally allocated by thread B).
- Maintain per-thread retired memory lists.
- Scan global hazard pointers before physically returning unmapped virtual pages to the OS via `munmap` or resetting slab ownership.

### 4. Cacheline Padding & False-Sharing Prevention
- Pad thread-local structures and central bucket heads to `hardware_destructive_interference_size` (64 bytes).
- Ensure no shared mutable atomic counters reside in the same 64-byte cache line.

## Build and Test Instructions
The project uses CMake and C++20:
```bash
cd /app
mkdir -p build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build . --parallel $(nproc)
ctest --output-on-failure
```

The verifier executes `tests/test.sh`, which runs multi-threaded allocation stress benchmarks, producer-consumer queues (64 threads), fragmentation audits, and ThreadSanitizer/AddressSanitizer checks.
