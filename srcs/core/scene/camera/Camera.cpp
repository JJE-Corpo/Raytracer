//
// Created by jazema on 4/21/26.
//

#include "Camera.hpp"

#include <cmath>

#include "../../../common/Utils.hpp"

namespace rc
{
    Camera::Camera(const Vector2i &resolution, const Vector3f &position, const Vector3f &rotation, float fov, int samplesPerPixel) : _resolution(resolution), _position(position), _rotation(rotation), _fov(fov), samples_per_pixel(samplesPerPixel)
    {
        if (this->samples_per_pixel <= 0)
            this->samples_per_pixel = 1;
        Camera::update();
    }

    Vector2i Camera::getResolution() const
    {
        return (this->_resolution);
    }

    Vector3f Camera::getPosition() const
    {
        return (this->_position);
    }

    Vector3f Camera::getRotation() const
    {
        return (this->_rotation);
    }

    double Camera::getFov() const
    {
        return (this->_fov);
    }

    int Camera::getSamplesPerPixel() const
    {
        return (this->samples_per_pixel);
    }

    double Camera::getPixelSamplesScale() const
    {
        return (this->pixel_samples_scale);
    }

    void Camera::setResolution(const Vector2i &resolution)
    {
        this->_resolution = resolution;
        this->update();
    }

    void Camera::setPosition(const Vector3f &position)
    {
        this->_position = position;
        this->update();
    }

    void Camera::setRotation(const Vector3f &rotation)
    {
        this->_rotation = rotation;
        this->update();
    }

    void Camera::setFov(double fov)
    {
        this->_fov = fov;
        this->update();
    }

    void Camera::setSamplesPerPixel(int spp)
    {
        this->samples_per_pixel = spp;
        
        if (this->samples_per_pixel <= 0)
            this->samples_per_pixel = 1;
        
        this->pixel_samples_scale = 1.0 / this->samples_per_pixel;
    }

    Vector3f Camera::sample_square() const
    {
        return Vector3f(Utils::random_double() - 0.5, Utils::random_double() - 0.5, 0);
    }

    Vector3f Camera::getForward() const
    {
        const Vector3f rotation_radians(
            Utils::degrees_to_radians(this->_rotation.x),
            Utils::degrees_to_radians(this->_rotation.y),
            Utils::degrees_to_radians(this->_rotation.z)
        );

        return rotate(Vector3f(0, 1, 0), rotation_radians).unit_vector();
    }

    Vector3f Camera::getRight() const
    {
        const Vector3f rotation_radians(
            Utils::degrees_to_radians(this->_rotation.x),
            Utils::degrees_to_radians(this->_rotation.y),
            Utils::degrees_to_radians(this->_rotation.z)
        );

        return rotate(Vector3f(1, 0, 0), rotation_radians).unit_vector();
    }

    Vector3f Camera::getUp() const
    {
        const Vector3f rotation_radians(
            Utils::degrees_to_radians(this->_rotation.x),
            Utils::degrees_to_radians(this->_rotation.y),
            Utils::degrees_to_radians(this->_rotation.z)
        );

        return rotate(Vector3f(0, 0, 1), rotation_radians).unit_vector();
    }

    void Camera::update()
    {
        const float focal_length = 1.0f;
        const float pi = 3.14159265358979323846f;
        const float theta = this->_fov * pi / 180.0f;
        const float viewport_height = 2.0f * std::tan(theta / 2.0f);
        const float viewport_width = viewport_height * (static_cast<float>(this->_resolution.x) / static_cast<float>(this->_resolution.y));

        const Vector3f rotation_radians(
            Utils::degrees_to_radians(this->_rotation.x),
            Utils::degrees_to_radians(this->_rotation.y),
            Utils::degrees_to_radians(this->_rotation.z)
        );

        const Vector3f forward = rotate(Vector3f(0, 1, 0), rotation_radians).unit_vector();
        const Vector3f right = rotate(Vector3f(1, 0, 0), rotation_radians).unit_vector();
        const Vector3f up = rotate(Vector3f(0, 0, 1), rotation_radians).unit_vector();

        const Vector3f viewport_u = viewport_width * right;
        const Vector3f viewport_v = -viewport_height * up;

        this->pixel_delta_u = viewport_u / static_cast<float>(this->_resolution.x);
        this->pixel_delta_v = viewport_v / static_cast<float>(this->_resolution.y);

        const Vector3f viewport_upper_left = this->_position + focal_length * forward - viewport_u / 2.0f - viewport_v / 2.0f;
        this->pixel00_loc = viewport_upper_left + 0.5f * (this->pixel_delta_u + this->pixel_delta_v);
        this->pixel_samples_scale = 1.0 / this->samples_per_pixel;
    }

    Ray Camera::generateRay(int i, int j) const
    {
        auto offset = sample_square();
        auto pixel_sample = pixel00_loc
                          + ((i + offset.x) * pixel_delta_u)
                          + ((j + offset.y) * pixel_delta_v);

        auto ray_origin = this->_position;
        auto ray_direction = (pixel_sample - ray_origin).unit_vector();

        return (Ray(ray_origin, ray_direction));
    }
}
