# Differentiable Sparse Voxel Octree Renderer with Analytical Radiance Gradients

## Overview
Your objective is to implement a high-performance, differentiable Sparse Voxel Octree (SVO) ray-tracing renderer in C++20 with AVX2 / OpenMP acceleration and analytical backward gradient computation for 3D radiance fields in `/app`.

## Architecture & Requirements

### 1. Morton-Order Spatial Hashing & Octree Topology
- Interleave 3D integer coordinates $(x, y, z)$ into 64-bit Morton codes (Z-order curve) using bitwise dilation.
- Store sparse octree nodes with an 8-bit child mask and 32-bit child pointer offsets:
  - Leaf nodes store voxel density $\sigma \ge 0$ and spherical harmonics (SH degree 2, 9 coefficients per RGB channel $\implies$ 27 floats).

### 2. Hierarchical DDA Ray Marching & Alpha Compositing
- Cast camera rays $\mathbf{r}(t) = \mathbf{o} + t \mathbf{d}$ through the sparse octree:
  - Digital Differential Analyzer (DDA) stepping with empty space skipping.
  - Trilinear interpolation of density and color across neighboring voxel vertices.
  - Volumetric alpha-compositing:
    $$T_i = \exp\left(-\sum_{j=1}^{i-1} \sigma_j \delta_j\right), \quad \alpha_i = 1 - \exp(-\sigma_i \delta_i), \quad C(\mathbf{r}) = \sum_{i=1}^N T_i \alpha_i \mathbf{c}_i$$

### 3. Analytical Differentiable Backward Pass
- Given image loss $\mathcal{L} = \frac{1}{2} \sum \|C(\mathbf{r}) - C_{\text{gt}}(\mathbf{r})\|^2$, compute exact analytical derivatives:
  - $\frac{\partial \mathcal{L}}{\partial \mathbf{c}_i} = \frac{\partial \mathcal{L}}{\partial C} \cdot T_i \alpha_i$
  - $\frac{\partial \mathcal{L}}{\partial \sigma_i} = \frac{\partial \mathcal{L}}{\partial C} \cdot \left[ T_i (1 - \alpha_i) \delta_i \mathbf{c}_i - \delta_i \sum_{k=i+1}^N T_k \alpha_k \mathbf{c}_k \right]$
- Match finite-difference numerical gradients ($L_\infty < 10^{-3}$) without storing intermediate activation volumes.

## Build and Test Instructions
The project uses CMake and C++20:
```bash
cd /app
mkdir -p build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build . --parallel $(nproc)
ctest --output-on-failure
```

The verifier executes `tests/test.sh`, which evaluates Morton encoding, numerical vs. analytical gradient accuracy, multi-view 3D reconstruction PSNR ($\ge 28.0\text{ dB}$), and render FPS ($\ge 40\text{ FPS}$).
