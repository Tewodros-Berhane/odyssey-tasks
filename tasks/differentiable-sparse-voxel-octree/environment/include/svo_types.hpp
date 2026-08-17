#pragma once
#include <cstdint>
#include <vector>
#include <array>
#include <cmath>

namespace svo {

struct Vec3 {
    float x{0.0f}, y{0.0f}, z{0.0f};

    Vec3 operator+(const Vec3& o) const { return {x + o.x, y + o.y, z + o.z}; }
    Vec3 operator-(const Vec3& o) const { return {x - o.x, y - o.y, z - o.z}; }
    Vec3 operator*(float s) const { return {x * s, y * s, z * s}; }
};

struct Ray {
    Vec3 origin;
    Vec3 direction;
};

struct OctreeNode {
    uint8_t child_mask{0};
    uint32_t child_offset{0};
    float density{0.0f};
    std::array<float, 27> sh_coeffs{}; // Degree 2 Spherical Harmonics (3 channels * 9)
};

} // namespace svo
