//
// Created by jazema on 4/21/26.
//

#ifndef CAMERA_HPP
#define CAMERA_HPP
#include "../../../common/scene/ICamera.hpp"
#include "../../../common/Ray.hpp"
#include "../../../common/Vector.hpp"

namespace rc
{
    class Camera : public ICamera
    {
        private:
            Vector2i _resolution;
            Vector3f _position;
            Vector3f _rotation;
            double _fov;
            int samples_per_pixel;
            // int max_depth = 10;
            double pixel_samples_scale;
            Vector3f pixel00_loc;
            Vector3f pixel_delta_u;
            Vector3f pixel_delta_v;

            Vector3f sample_square() const;
        public:
            Camera(const Vector2i &resolution, const Vector3f &position, const Vector3f &rotation, float fov, int samplesPerPixel = 5);

            Vector2i getResolution() const override;
            Vector3f getPosition() const override;
            Vector3f getRotation() const override;
            double getFov() const override;

            Vector3f getForward() const override;
            Vector3f getRight() const override;
            Vector3f getUp() const;

            void update();

            int getSamplesPerPixel() const override;
            double getPixelSamplesScale() const override;

            void setResolution(const Vector2i &resolution) override;
            void setPosition(const Vector3f &position) override;
            void setRotation(const Vector3f &rotation) override;
            void setFov(double fov) override;
            void setSamplesPerPixel(int spp) override;

            Ray generateRay(int i, int j) const override;
    };
}

#endif
