# Paged FlashAttention-2 Custom Kernel with Backward Pass and Variable-Length Packing

## Overview
Your objective is to implement a high-performance, custom FlashAttention-2 GPU kernel in OpenAI Triton supporting both Forward and Backward gradient passes, causal masking, variable-length sequence packing (`cu_seqlens`), and Paged KV-Cache in `/app`.

## Architecture & Requirements

### 1. Forward Pass with Online Softmax Rescaling
Implement the tiled matrix multiplication and online softmax algorithm:
- Tile Query ($Q$) into blocks of size $B_r \times d$, and Key/Value ($K, V$) into blocks of size $B_c \times d$.
- Maintain running softmax statistics: row maximum $m_i$ and denominator sum $l_i$ in GPU SRAM registers.
- Online rescaling update:
  $$m_{\text{new}} = \max(m_{\text{prev}}, \text{rowmax}(S_{ij}))$$
  $$\tilde{P}_{ij} = \exp(S_{ij} - m_{\text{new}})$$
  $$l_{\text{new}} = \exp(m_{\text{prev}} - m_{\text{new}}) l_{\text{prev}} + \text{rowsum}(\tilde{P}_{ij})$$
  $$O_{\text{new}} = \text{diag}(\exp(m_{\text{prev}} - m_{\text{new}})) O_{\text{prev}} + \tilde{P}_{ij} V_j$$
- Final normalization: $O = \text{diag}(l_{\text{final}})^{-1} O_{\text{final}}$.

### 2. Backward Pass Gradient Mathematics
Implement the autograd backward pass for $dQ, dK, dV$:
- Recompute attention probabilities $\tilde{P}_{ij}$ on-the-fly from $Q_i, K_j$ and stored logsumexp values $L = m + \log l$.
- Compute intermediate gradients:
  $$dV_j = \sum_i P_{ij}^T dO_i$$
  $$dP_{ij} = dO_i V_j^T$$
  $$D_i = \text{rowsum}(dO_i \odot O_i)$$
  $$dS_{ij} = P_{ij} \odot (dP_{ij} - D_i)$$
  $$dQ_i = \frac{1}{\sqrt{d}} \sum_j dS_{ij} K_j$$
  $$dK_j = \frac{1}{\sqrt{d}} \sum_i dS_{ij}^T Q_i$$

### 3. Paged KV-Cache Memory Indirection
- Implement support for non-contiguous physical page tables (vLLM-style paged memory):
  - `block_tables`: `Tensor[batch_size, max_num_blocks_per_seq]` of physical block indices.
  - `key_cache`, `value_cache`: `Tensor[num_blocks, block_size, num_heads, head_dim]`.
  - Kernel must compute physical block pointers per token position and load KV tiles without memory reallocation.

## Build and Test Instructions
The environment contains PyTorch and Triton:
```bash
cd /app
pytest tests/
```

The verifier executes `tests/test.sh`, which benchmarks FP16 forward/backward numerical accuracy, autograd gradients, paged KV-cache serving, and TFLOPS speedup.
