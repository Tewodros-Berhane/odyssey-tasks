# AVX-512 / AVX2 Accelerated 32-Qubit Quantum State-Vector Simulator with Gate Fusion

## Overview
Your objective is to implement an ultra-fast, multi-threaded 32-qubit state-vector quantum circuit simulator in C++20 accelerated by hand-optimized AVX2 / AVX-512 complex arithmetic intrinsics and DAG-based unitary gate fusion in `/app`.

## Architecture & Requirements

### 1. State-Vector Memory & Complex Arithmetic Representation
- Represent $N$-qubit quantum states as a contiguous array of $2^N$ complex numbers:
  $$\vert \psi \rangle = \sum_{k=0}^{2^N-1} (\alpha_k + i \beta_k) \vert k \rangle$$
- Implement cache-aligned (`alignas(64)`) complex float32 / float64 buffer allocations.
- Implement AVX2 / AVX-512 SIMD complex multiplication:
  $$(a + ib)(c + id) = (ac - bd) + i(ad + bc)$$
  using `_mm256_fmadd_ps`, `_mm256_fmsub_ps`, `_mm256_permute_ps`, `_mm256_hadd_ps`.

### 2. Quantum Gate Application
- Single-Qubit Gates:
  - Hadamard ($H$), Pauli ($X, Y, Z$), Phase ($S, T$).
  - Arbitrary 1-qubit rotation $U3(\theta, \phi, \lambda)$ / $R_x(\theta), R_y(\theta), R_z(\theta)$.
  - Stride-based permutation: target qubit $q$ splits amplitudes into pairs separated by stride $2^q$.
- Multi-Qubit Controlled Gates:
  - CNOT ($CX$), Controlled-Z ($CZ$), SWAP.
  - Multi-controlled unitaries ($MCX$, Toffoli).

### 3. Static Unitary Gate Fusion
- Analyze circuit DAGs before execution:
  - Identify chains of single-qubit gates on the same qubit and multiply matrices: $U_{\text{fused}} = U_k \dots U_2 U_1$.
  - Identify two-qubit gate pairs acting on the same pair of qubits and fuse into a single $4 \times 4$ unitary operator.
  - Apply fused $4 \times 4$ operators in a single pass over memory, reducing memory bandwidth by up to $4\times$.

### 4. Measurement & Sampling
- Compute probability distribution $P(k) = \vert \alpha_k \vert^2 + \vert \beta_k \vert^2$.
- Support single-qubit projective measurement with state collapse and normalization.
- Fast multi-qubit sampling (Monte Carlo alias method or parallel binary search).

## Build and Test Instructions
The project uses CMake and C++20:
```bash
cd /app
mkdir -p build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build . --parallel $(nproc)
ctest --output-on-failure
```

The verifier executes `tests/test.sh`, checking quantum circuit fidelity, QFT-8 precision, gate fusion speedup, and random circuit sampling.
