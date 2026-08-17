#pragma once
#include "pq.hpp"
#include <vector>
#include <shared_mutex>
#include <random>
#include <queue>
#include <span>

namespace vecengine {

using node_id_t = uint32_t;

struct SearchResult {
    node_id_t id;
    float distance;
    bool operator>(const SearchResult& other) const { return distance > other.distance; }
    bool operator<(const SearchResult& other) const { return distance < other.distance; }
};

class HNSWIndex {
public:
    HNSWIndex(size_t dim, size_t M = 32, size_t ef_construction = 128, size_t ef_search = 64);

    void BuildPQ(std::span<const float> train_data, size_t num_vectors, size_t num_subspaces = 32);
    void Insert(node_id_t id, std::span<const float> vector);
    std::vector<SearchResult> SearchKNN(std::span<const float> query, size_t k) const;

    size_t Size() const { return num_elements_; }

private:
    int SelectRandomLevel();

    size_t dim_;
    size_t M_;
    size_t M0_;
    size_t ef_construction_;
    size_t ef_search_;
    double ml_;
    
    std::unique_ptr<ProductQuantizer> pq_;
    std::vector<uint8_t> quantized_vectors_; // node_id * num_subspaces
    std::vector<std::vector<std::vector<node_id_t>>> graph_; // level -> node_id -> neighbors
    std::vector<int> node_levels_;

    node_id_t enter_node_{0};
    int max_level_{-1};
    size_t num_elements_{0};

    mutable std::shared_mutex global_mutex_;
    std::default_random_engine rng_{42};
};

} // namespace vecengine
