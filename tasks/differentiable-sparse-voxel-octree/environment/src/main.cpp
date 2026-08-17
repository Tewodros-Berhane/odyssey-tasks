#include "volume_renderer.hpp"
#include <iostream>

int main() {
    std::cout << "Starting Differentiable Sparse Voxel Octree Renderer..." << std::endl;
    auto octree = std::make_shared<svo::SparseVoxelOctree>(8);
    svo::VolumeRenderer renderer(octree);
    std::cout << "SVO renderer initialized." << std::endl;
    return 0;
}
