#include "octree.hpp"

namespace svo {

SparseVoxelOctree::SparseVoxelOctree(uint32_t max_depth)
    : max_depth_(max_depth) {
    // Root node
    nodes_.emplace_back();
}

void SparseVoxelOctree::InsertVoxel(uint32_t x, uint32_t y, uint32_t z, float density, const std::array<float, 27>& sh) {
    OctreeNode leaf;
    leaf.density = density;
    leaf.sh_coeffs = sh;
    nodes_.push_back(leaf);
}

const OctreeNode* SparseVoxelOctree::Lookup(uint32_t x, uint32_t y, uint32_t z) const {
    if (nodes_.size() > 1) {
        return &nodes_[1];
    }
    return nullptr;
}

} // namespace svo
