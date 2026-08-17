#include "simd_ops.hpp"
#include <immintrin.h>
#include <cmath>

namespace vecengine {

float SIMDOps::L2Distance(std::span<const float> a, std::span<const float> b) {
    size_t n = a.size();
    size_t i = 0;
    __m256 sum256 = _mm256_setzero_ps();

    for (; i + 8 <= n; i += 8) {
        __m256 va = _mm256_loadu_ps(&a[i]);
        __m256 vb = _mm256_loadu_ps(&b[i]);
        __m256 diff = _mm256_sub_ps(va, vb);
        sum256 = _mm256_fmadd_ps(diff, diff, sum256);
    }

    alignas(32) float buffer[8];
    _mm256_store_ps(buffer, sum256);
    float total = buffer[0] + buffer[1] + buffer[2] + buffer[3] + buffer[4] + buffer[5] + buffer[6] + buffer[7];

    for (; i < n; ++i) {
        float diff = a[i] - b[i];
        total += diff * diff;
    }
    return total;
}

float SIMDOps::DotProduct(std::span<const float> a, std::span<const float> b) {
    size_t n = a.size();
    size_t i = 0;
    __m256 sum256 = _mm256_setzero_ps();

    for (; i + 8 <= n; i += 8) {
        __m256 va = _mm256_loadu_ps(&a[i]);
        __m256 vb = _mm256_loadu_ps(&b[i]);
        sum256 = _mm256_fmadd_ps(va, vb, sum256);
    }

    alignas(32) float buffer[8];
    _mm256_store_ps(buffer, sum256);
    float total = buffer[0] + buffer[1] + buffer[2] + buffer[3] + buffer[4] + buffer[5] + buffer[6] + buffer[7];

    for (; i < n; ++i) {
        total += a[i] * b[i];
    }
    return total;
}

void SIMDOps::ComputeDistanceTable(
    std::span<const float> query,
    std::span<const float> centroids,
    size_t num_subspaces,
    size_t sub_dim,
    size_t num_centroids,
    std::span<float> distance_table
) {
    for (size_t m = 0; m < num_subspaces; ++m) {
        auto query_sub = query.subspan(m * sub_dim, sub_dim);
        for (size_t k = 0; k < num_centroids; ++k) {
            size_t c_offset = (m * num_centroids + k) * sub_dim;
            auto centroid_sub = centroids.subspan(c_offset, sub_dim);
            distance_table[m * num_centroids + k] = L2Distance(query_sub, centroid_sub);
        }
    }
}

} // namespace vecengine
