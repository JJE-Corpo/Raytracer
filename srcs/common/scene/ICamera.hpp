//
// Created by jazema on 4/22/26.
//

#ifndef ICAMERA_HPP
#define ICAMERA_HPP
#include "../Ray.hpp"
#include "../Vector.hpp"

namespace rc
{
    class ICamera
    {
        public:
            virtual ~ICamera() = default;

            virtual Vector2i getResolution() const = 0;
            virtual Vector3f getPosition() const = 0;
            virtual Vector3f getRotation() const = 0;
            virtual double getFov() const = 0;

            virtual Vector3f getForward() const = 0;
            virtual Vector3f getRight() const = 0;

            virtual int getSamplesPerPixel() const = 0;
            virtual double getPixelSamplesScale() const = 0;

            virtual void setResolution(const Vector2i &resolution) = 0;
            virtual void setPosition(const Vector3f &position) = 0;
            virtual void setRotation(const Vector3f &rotation) = 0;
            virtual void setFov(double fov) = 0;
            virtual void setSamplesPerPixel(int spp) = 0;

            virtual Ray generateRay(int i, int j) const = 0;
    };
}

#endif
