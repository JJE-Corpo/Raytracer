//
// Created by jazema on 4/21/26.
//

#ifndef RAY_HPP
#define RAY_HPP
#include "Vector.hpp"

namespace rc
{
    class Ray
    {
        public:
            Vector3f origin;
            Vector3f direction;

            Ray() = default;

            Ray(const Vector3f &origin, const Vector3f &direction): origin(origin), direction(direction)
            {
            }

            Vector3f at(float t) const
            {
                return (origin + t * direction);
            }
    };
}

#endif
