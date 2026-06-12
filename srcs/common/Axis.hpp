//
// Created by jazema on 4/30/26.
//

#ifndef AXIS_HPP
#define AXIS_HPP
#include "Vector.hpp"

namespace rc
{
    enum class Axis
    {
        X,
        Y,
        Z
    };
    class AxisUtils
    {
        public:
            static Vector3f toVector(const Axis &axis)
            {
                switch (axis)
                {
                    case Axis::X: return Vector3f(1.f, 0.f, 0.f);
                    case Axis::Y: return Vector3f(0.f, 1.f, 0.f);
                    case Axis::Z: return Vector3f(0.f, 0.f, 1.f);
                }
                return Vector3f(0.f, 0.f, 0.f);
            }
    };
}

#endif
