#pragma once
#include "svo_types.hpp"
#include "morton.hpp"
#include <vector>
#include <memory>

namespace svo {

class SparseVoxelOctree {
public:
    explicit SparseVoxelOctree(uint32_t max_depth = 8);

    void InsertVoxel(uint32_t x, uint32_t y, uint32_t z, float density, const std::array<float, 27>& sh);
    const OctreeNode* Lookup(uint32_t x, uint32_t y, uint32_t z) const;

    size_t NodeCount() const { return nodes_.size(); }
    std::vector<OctreeNode>& GetNodes() { return nodes_; }
    const std::vector<OctreeNode>& GetNodes() const { return nodes_; }

private:
    uint32_t max_depth_;
    std::vector<OctreeNode> nodes_;
};

} // namespace svo
