#include "hnsw.hpp"
#include <cmath>
#include <set>
#include <unordered_set>
#include <iostream>

namespace vecengine {

HNSWIndex::HNSWIndex(size_t dim, size_t M, size_t ef_construction, size_t ef_search)
    : dim_(dim), M_(M), M0_(2 * M), ef_construction_(ef_construction), ef_search_(ef_search), ml_(1.0 / std::log(static_cast<double>(M))) {}

int HNSWIndex::SelectRandomLevel() {
    std::uniform_real_distribution<double> dist(0.0, 1.0);
    double r = dist(rng_);
    if (r == 0.0) r = 0.0000001;
    return static_cast<int>(-std::log(r) * ml_);
}

void HNSWIndex::BuildPQ(std::span<const float> train_data, size_t num_vectors, size_t num_subspaces) {
    pq_ = std::make_unique<ProductQuantizer>(dim_, num_subspaces);
    pq_->Train(train_data, num_vectors);
}

void HNSWIndex::Insert(node_id_t id, std::span<const float> vector) {
    std::unique_lock lock(global_mutex_);
    if (!pq_) {
        pq_ = std::make_unique<ProductQuantizer>(dim_, 32);
    }
    auto code = pq_->Encode(vector);
    quantized_vectors_.insert(quantized_vectors_.end(), code.begin(), code.end());
    int level = SelectRandomLevel();

    if (level >= static_cast<int>(graph_.size())) {
        graph_.resize(level + 1);
    }
    for (int l = 0; l <= level; ++l) {
        if (id >= graph_[l].size()) {
            graph_[l].resize(id + 1);
        }
    }

    node_levels_.push_back(level);
    if (num_elements_ == 0) {
        enter_node_ = id;
        max_level_ = level;
    } else {
        // Connect to entry node on all assigned levels
        for (int l = 0; l <= std::min(level, max_level_); ++l) {
            graph_[l][id].push_back(enter_node_);
            graph_[l][enter_node_].push_back(id);
        }
        if (level > max_level_) {
            max_level_ = level;
            enter_node_ = id;
        }
    }
    num_elements_++;
}

std::vector<SearchResult> HNSWIndex::SearchKNN(std::span<const float> query, size_t k) const {
    std::shared_lock lock(global_mutex_);
    if (num_elements_ == 0 || !pq_) return {};

    auto q_table = pq_->ComputeQueryTable(query);
    size_t num_subspaces = pq_->GetNumSubspaces();

    std::priority_queue<SearchResult> top_candidates;
    for (node_id_t id = 0; id < num_elements_; ++id) {
        const uint8_t* code_ptr = &quantized_vectors_[id * num_subspaces];
        float dist = pq_->AsymmetricDistance(q_table, code_ptr);
        top_candidates.push({id, dist});
        if (top_candidates.size() > k) {
            top_candidates.pop();
        }
    }

    std::vector<SearchResult> results;
    while (!top_candidates.empty()) {
        results.push_back(top_candidates.top());
        top_candidates.pop();
    }
    std::reverse(results.begin(), results.end());
    return results;
}

} // namespace vecengine
