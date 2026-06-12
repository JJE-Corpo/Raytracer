//
// Created by jazema on 4/30/26.
//

#ifndef VECTORUTILS_HPP
#define VECTORUTILS_HPP
#include "Vector.hpp"

namespace rc
{
    class VectorUtils
    {
        public:
            static Vector3f random_unit_vector()
            {
                while (true) {
                    auto p = Vector3f::random(-1,1);
                    auto lensq = p.length_squared();
                    if (1e-160 < lensq && lensq <= 1)
                        return p / sqrt(lensq);
                }
            }

            static Vector3f random_on_hemisphere(const Vector3f &normal)
            {
                Vector3f on_unit_sphere = random_unit_vector();
                if (dot(on_unit_sphere, normal) > 0.0)
                    return (on_unit_sphere);
                return (-on_unit_sphere);
            }
    };
}

#endif
