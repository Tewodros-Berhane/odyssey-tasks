# Zero-Copy WASM Core Engine with Tier-1 JIT Compiler and Spec Conformance

## Overview
Your objective is to implement a high-performance WebAssembly (WASM MVP) core runtime engine in C++20 featuring a streaming binary parser/validator and a Tier-1 baseline JIT compiler generating native x86-64 machine code in `/app`.

## Architecture & Requirements

### 1. WASM Binary Decoding & LEB128
- Implement unsigned and signed LEB128 integer decoders (`varuint32`, `varint32`, `varint64`).
- Parse standard WASM binary sections:
  - `Type` section (Function signatures: parameters and return value types).
  - `Import` / `Export` sections (Functions, tables, memory, globals).
  - `Function` section (Type indices for module functions).
  - `Table` / `Memory` section (Initial and maximum page limits: 1 page = 64 KiB).
  - `Global` section (Mutable and immutable global variables).
  - `Code` section (Function locals and raw opcode byte streams).

### 2. Tier-1 x86-64 Baseline JIT Compiler
- Allocate executable memory pages using `mmap(NULL, size, PROT_READ | PROT_WRITE | PROT_EXEC, MAP_ANONYMOUS | MAP_PRIVATE, -1, 0)`.
- Single-pass machine code generation:
  - Maintain a virtual evaluation stack during compilation to track value types.
  - Map WASM stack operations to x86-64 instructions (`mov`, `add`, `sub`, `imul`, `idiv`, `and`, `or`, `xor`, `shl`, `shr`, `sar`).
  - Implement structured control flow:
    - `block` / `loop` / `if` / `else` / `end`: Maintain compile-time label stack and patch forward/backward jump targets.
    - `br` / `br_if` / `br_table`: Emit conditional and indirect branch instructions.
  - Calling conventions: Conform to System V AMD64 ABI for function calls and host interop.

### 3. Linear Memory & Bounds Safety
- Support `i32.load`, `i64.load`, `i32.store`, `i64.store`, `memory.size`, `memory.grow`.
- Bounds enforcement: Verify `effective_address + access_size <= current_memory_bytes` before loads/stores, emitting runtime trap handlers on violation.

## Build and Test Instructions
The project uses CMake and C++20:
```bash
cd /app
mkdir -p build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build . --parallel $(nproc)
ctest --output-on-failure
```

The verifier executes `tests/test.sh`, which evaluates binary parsing, W3C spec conformance, native JIT execution, and trap safety.
