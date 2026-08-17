#pragma once
#include <cstddef>
#include <span>
#include <cstdint>

namespace vecengine {

class SIMDOps {
public:
    static float L2Distance(std::span<const float> a, std::span<const float> b);
    static float DotProduct(std::span<const float> a, std::span<const float> b);
    static void ComputeDistanceTable(
        std::span<const float> query,
        std::span<const float> centroids,
        size_t num_subspaces,
        size_t sub_dim,
        size_t num_centroids,
        std::span<float> distance_table
    );
};

} // namespace vecengine
