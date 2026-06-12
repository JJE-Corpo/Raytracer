//
// Created by jazema on 5/1/26.
//

#ifndef AABB_HPP
#define AABB_HPP
#include "Ray.hpp"
#include "Vector.hpp"

namespace rc
{
    struct AABB
    {
        Vector3f min;
        Vector3f max;

        bool hit(const Ray &r, float tMin, float tMax) const
        {
            for (int a = 0; a < 3; a++)
            {
                float invD = 1.0f / r.direction[a];
                float t0 = (this->min[a] - r.origin[a]) * invD;
                float t1 = (this->max[a] - r.origin[a]) * invD;

                if (invD < 0.0f) std::swap(t0, t1);

                tMin = t0 > tMin ? t0 : tMin;
                tMax = t1 < tMax ? t1 : tMax;

                // Strict less-than: AABB plates (min == max sur un axe) restent valides
                if (tMax < tMin)
                    return (false);
            }
            return (true);
        }
    };
    class BoundingBoxUtils
    {
        public:
            static AABB surrounding_box(const AABB& box0, const AABB& box1)
            {
                Vector3f small(
                    std::min(box0.min.x, box1.min.x),
                    std::min(box0.min.y, box1.min.y),
                    std::min(box0.min.z, box1.min.z)
                );

                Vector3f big(
                    std::max(box0.max.x, box1.max.x),
                    std::max(box0.max.y, box1.max.y),
                    std::max(box0.max.z, box1.max.z)
                );

                return AABB{small, big};
            }
    };
}

#endif
