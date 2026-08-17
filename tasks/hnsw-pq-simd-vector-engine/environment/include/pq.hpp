#pragma once
#include <vector>
#include <cstdint>
#include <span>
#include <cstddef>

namespace vecengine {

class ProductQuantizer {
public:
    ProductQuantizer(size_t dim, size_t num_subspaces, size_t num_centroids = 256);

    void Train(std::span<const float> training_data, size_t num_vectors, size_t max_iters = 20);
    std::vector<uint8_t> Encode(std::span<const float> vector) const;
    std::vector<float> ComputeQueryTable(std::span<const float> query) const;
    float AsymmetricDistance(const std::vector<float>& query_table, const uint8_t* quantized_code) const;

    size_t GetDim() const { return dim_; }
    size_t GetNumSubspaces() const { return num_subspaces_; }
    size_t GetSubDim() const { return sub_dim_; }
    size_t GetNumCentroids() const { return num_centroids_; }
    const std::vector<float>& GetCentroids() const { return centroids_; }

private:
    size_t dim_;
    size_t num_subspaces_;
    size_t sub_dim_;
    size_t num_centroids_;
    std::vector<float> centroids_; // num_subspaces * num_centroids * sub_dim
};

} // namespace vecengine
