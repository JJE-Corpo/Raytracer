//
// Small helpers to derive texture UV coordinates from geometry, shared by the
// primitives so texture mapping stays consistent across shapes.
//

#ifndef UVMAPPING_HPP
#define UVMAPPING_HPP

#include <algorithm>
#include <cmath>

#include "Vector.hpp"

namespace rc::uvmap
{
    constexpr float PI = 3.14159265358979323846f;

    // Two orthonormal vectors spanning the plane perpendicular to `n`.
    inline void basis(const Vector3f &n, Vector3f &tangent, Vector3f &bitangent)
    {
        Vector3f nn = normalize(n);
        Vector3f up = (std::fabs(nn.y) > 0.99f) ? Vector3f(1.0f, 0.0f, 0.0f) : Vector3f(0.0f, 1.0f, 0.0f);
        tangent = normalize(up.cross(nn));
        bitangent = nn.cross(tangent);
    }

    // Latitude/longitude mapping of a unit direction onto [0,1]^2.
    inline Vector2f sphere(const Vector3f &dir)
    {
        Vector3f d = normalize(dir);
        float phi = std::atan2(d.z, d.x);
        float theta = std::asin(std::max(-1.0f, std::min(1.0f, d.y)));
        return Vector2f(0.5f + phi / (2.0f * PI), 0.5f + theta / PI);
    }

    // Planar projection of `rel` (point relative to an origin) onto the plane
    // whose normal is `n`, divided by `scale` for tiling.
    inline Vector2f planar(const Vector3f &rel, const Vector3f &n, float scale)
    {
        Vector3f tangent;
        Vector3f bitangent;
        basis(n, tangent, bitangent);
        float s = (scale != 0.0f) ? scale : 1.0f;
        return Vector2f(dot(rel, tangent) / s, dot(rel, bitangent) / s);
    }

    // Angle-around-axis (u) + height-along-axis (v) mapping, for cylinders/cones.
    inline Vector2f cylindrical(const Vector3f &rel, const Vector3f &axis, float height)
    {
        Vector3f tangent;
        Vector3f bitangent;
        basis(axis, tangent, bitangent);
        Vector3f a = normalize(axis);
        float h = dot(rel, a);
        Vector3f radial = rel - a * h;
        float ang = std::atan2(dot(radial, bitangent), dot(radial, tangent));
        return Vector2f(0.5f + ang / (2.0f * PI), (height != 0.0f) ? h / height : h);
    }
}

#endif
