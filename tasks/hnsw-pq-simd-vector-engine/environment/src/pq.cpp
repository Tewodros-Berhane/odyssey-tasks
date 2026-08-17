#include "pq.hpp"
#include "simd_ops.hpp"
#include <algorithm>
#include <random>

namespace vecengine {

ProductQuantizer::ProductQuantizer(size_t dim, size_t num_subspaces, size_t num_centroids)
    : dim_(dim), num_subspaces_(num_subspaces), sub_dim_(dim / num_subspaces), num_centroids_(num_centroids) {
    centroids_.resize(num_subspaces_ * num_centroids_ * sub_dim_, 0.0f);
}

void ProductQuantizer::Train(std::span<const float> training_data, size_t num_vectors, size_t max_iters) {
    std::default_random_engine rng(1337);
    for (size_t m = 0; m < num_subspaces_; ++m) {
        // Initialize centroids randomly from training data
        for (size_t k = 0; k < num_centroids_; ++k) {
            size_t rand_vec_idx = rng() % num_vectors;
            for (size_t d = 0; d < sub_dim_; ++d) {
                centroids_[(m * num_centroids_ + k) * sub_dim_ + d] =
                    training_data[rand_vec_idx * dim_ + m * sub_dim_ + d];
            }
        }

        // K-Means iterations
        std::vector<size_t> counts(num_centroids_, 0);
        std::vector<float> new_centroids(num_centroids_ * sub_dim_, 0.0f);

        for (size_t iter = 0; iter < max_iters; ++iter) {
            std::fill(counts.begin(), counts.end(), 0);
            std::fill(new_centroids.begin(), new_centroids.end(), 0.0f);

            for (size_t i = 0; i < num_vectors; ++i) {
                auto sub_vec = training_data.subspan(i * dim_ + m * sub_dim_, sub_dim_);
                float best_dist = std::numeric_limits<float>::max();
                size_t best_k = 0;

                for (size_t k = 0; k < num_centroids_; ++k) {
                    auto c_vec = std::span<const float>(&centroids_[(m * num_centroids_ + k) * sub_dim_], sub_dim_);
                    float dist = SIMDOps::L2Distance(sub_vec, c_vec);
                    if (dist < best_dist) {
                        best_dist = dist;
                        best_k = k;
                    }
                }

                counts[best_k]++;
                for (size_t d = 0; d < sub_dim_; ++d) {
                    new_centroids[best_k * sub_dim_ + d] += sub_vec[d];
                }
            }

            for (size_t k = 0; k < num_centroids_; ++k) {
                if (counts[k] > 0) {
                    for (size_t d = 0; d < sub_dim_; ++d) {
                        centroids_[(m * num_centroids_ + k) * sub_dim_ + d] = new_centroids[k * sub_dim_ + d] / counts[k];
                    }
                }
            }
        }
    }
}

std::vector<uint8_t> ProductQuantizer::Encode(std::span<const float> vector) const {
    std::vector<uint8_t> codes(num_subspaces_);
    for (size_t m = 0; m < num_subspaces_; ++m) {
        auto sub_vec = vector.subspan(m * sub_dim_, sub_dim_);
        float best_dist = std::numeric_limits<float>::max();
        uint8_t best_k = 0;

        for (size_t k = 0; k < num_centroids_; ++k) {
            auto c_vec = std::span<const float>(&centroids_[(m * num_centroids_ + k) * sub_dim_], sub_dim_);
            float dist = SIMDOps::L2Distance(sub_vec, c_vec);
            if (dist < best_dist) {
                best_dist = dist;
                best_k = static_cast<uint8_t>(k);
            }
        }
        codes[m] = best_k;
    }
    return codes;
}

std::vector<float> ProductQuantizer::ComputeQueryTable(std::span<const float> query) const {
    std::vector<float> table(num_subspaces_ * num_centroids_);
    SIMDOps::ComputeDistanceTable(query, centroids_, num_subspaces_, sub_dim_, num_centroids_, table);
    return table;
}

float ProductQuantizer::AsymmetricDistance(const std::vector<float>& query_table, const uint8_t* quantized_code) const {
    float dist = 0.0f;
    for (size_t m = 0; m < num_subspaces_; ++m) {
        uint8_t code = quantized_code[m];
        dist += query_table[m * num_centroids_ + code];
    }
    return dist;
}

} // namespace vecengine
