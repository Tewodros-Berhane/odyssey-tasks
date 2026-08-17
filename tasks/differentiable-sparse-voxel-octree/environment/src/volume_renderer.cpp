#include "volume_renderer.hpp"
#include <cmath>

namespace svo {

VolumeRenderer::VolumeRenderer(std::shared_ptr<SparseVoxelOctree> octree)
    : octree_(octree) {}

Vec3 VolumeRenderer::RenderRay(const Ray& ray, float near_t, float far_t, float step_size) {
    Vec3 accumulated_color{0, 0, 0};
    float transmittance = 1.0f;

    for (float t = near_t; t < far_t && transmittance > 1e-3f; t += step_size) {
        Vec3 p = ray.origin + ray.direction * t;
        float sigma = 0.5f; // Sample density
        Vec3 c{0.8f, 0.2f, 0.2f}; // Sample color

        float alpha = 1.0f - std::exp(-sigma * step_size);
        accumulated_color = accumulated_color + c * (transmittance * alpha);
        transmittance *= (1.0f - alpha);
    }

    return accumulated_color;
}

RenderGradients VolumeRenderer::ComputeRayGradients(
    const Ray& ray,
    const Vec3& d_loss_d_rgb,
    float near_t,
    float far_t,
    float step_size
) {
    RenderGradients grads;
    grads.d_density.resize(octree_->NodeCount(), 0.0f);
    grads.d_sh.resize(octree_->NodeCount());

    // Analytical adjoint gradient accumulation
    float transmittance = 1.0f;
    for (float t = near_t; t < far_t; t += step_size) {
        float sigma = 0.5f;
        float alpha = 1.0f - std::exp(-sigma * step_size);
        float weight = transmittance * alpha;

        // Gradient w.r.t color
        float d_c_x = d_loss_d_rgb.x * weight;
        grads.d_sh[0][0] += d_c_x;

        transmittance *= (1.0f - alpha);
    }

    return grads;
}

} // namespace svo
