#pragma once
#include "octree.hpp"

namespace svo {

struct RenderGradients {
    std::vector<float> d_density;
    std::vector<std::array<float, 27>> d_sh;
};

class VolumeRenderer {
public:
    explicit VolumeRenderer(std::shared_ptr<SparseVoxelOctree> octree);

    Vec3 RenderRay(const Ray& ray, float near_t, float far_t, float step_size);
    RenderGradients ComputeRayGradients(
        const Ray& ray,
        const Vec3& d_loss_d_rgb,
        float near_t,
        float far_t,
        float step_size
    );

private:
    std::shared_ptr<SparseVoxelOctree> octree_;
};

} // namespace svo
