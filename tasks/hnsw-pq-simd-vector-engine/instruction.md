# AVX-512 / AVX2 Accelerated HNSW Vector Graph with Product Quantization

## Overview
Your objective is to complete a high-performance Hierarchical Navigable Small World (HNSW) vector search engine in C++20 accelerated by Product Quantization (PQ) and hand-tuned SIMD intrinsics (AVX2 / AVX-512 with scalar fallback) located in `/app`.

## Architecture & Requirements

### 1. Product Quantization (PQ) & Asymmetric Distance Computation (ADC)
- Implement Product Quantization for $D = 768$ dimensional floating-point vectors:
  - Subspace decomposition: Split vectors into $M = 32$ or $M = 48$ sub-vectors of dimension $d_{sub} = D / M$.
  - Quantization Codebook: $k = 256$ centroids per subspace (8 bits per sub-vector).
  - Precompute query-to-centroid distance lookup tables ($M \times 256$).
  - Asymmetric Distance Computation (ADC): Sum distance lookups across sub-vectors using SIMD instructions (`_mm256_loadu_ps` / `_mm256_add_ps` / `_mm256_dp_ps` or integer shuffle tables).

### 2. Multi-Layer HNSW Graph Construction & Search
- Implement the HNSW graph indexing structure:
  - Probability distribution for layer allocation ($l = \lfloor -\ln(\text{uniform}(0,1)) \cdot m_L \rfloor$).
  - Fast greedy routing from entry point down to top layer.
  - Greedy/Beam search with dynamic candidate queue of size `efSearch` on layer 0.
  - Heuristic neighbor selection (Diverse Neighbor Heuristic - Algorithm 4) to ensure connected graph topology across dense clusters.
  - Bidirectional edge updates with capacity limit `M_max0` on layer 0 and `M_max` on upper layers.

### 3. Concurrency & Performance Optimization
- Thread-safe concurrent insertions and lock-free read-side k-NN traversals.
- Cache-line alignment (`alignas(64)`) for vector centroids and distance lookup tables.
- Hardware prefetching (`_mm_prefetch`) for neighbor node coordinates during graph traversal.

## Target Performance Targets
On 768-dimensional benchmark datasets (8 vCPUs):
- **Recall@10**: $\ge 95\%$ compared to exact brute-force Euclidean distance.
- **Throughput**: $\ge 10,000$ queries per second (QPS).
- **Latency**: $p99 < 1.5\text{ ms}$.

## Build and Test Instructions
The project uses CMake and C++20:
```bash
cd /app
mkdir -p build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build . --parallel $(nproc)
ctest --output-on-failure
```

The verifier executes `tests/test.sh`, which evaluates recall, query throughput, concurrency correctness, and memory sanitizers.
