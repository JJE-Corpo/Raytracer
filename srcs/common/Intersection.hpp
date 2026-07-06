//
// Created by jazema on 4/21/26.
//

#ifndef RAYTRACER_INTERSECTION_HPP
#define RAYTRACER_INTERSECTION_HPP
#include "Ray.hpp"
#include "Vector.hpp"
#include "Material.hpp"

namespace rc
{
    class IPrimitive;

    struct Intersection
    {
        float t;
        Vector3f point;
        Vector3f normal;
        bool front_face;
        Vector2f uv = {0.0f, 0.0f};
        // ColorF color;
        Material material;
        const IPrimitive *primitive = nullptr;

        void set_face_normal(const Ray &r, const Vector3f &outward_normal) {
            this->front_face = dot(r.direction, outward_normal) < 0;
            this->normal = this->front_face ? outward_normal : -outward_normal;
        }
    };
}

#endif
