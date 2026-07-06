//
// Created by jazema on 4/30/26.
//

#include "Sphere.hpp"

#include <iostream>

#include "../../common/Intersection.hpp"
#include "../../common/Ray.hpp"
#include "../../core/scene/builder/SceneObjectBuilder.hpp"

namespace rc
{
    Sphere::Sphere(std::string name, const Vector3f &center, const Vector3f &scale, float radius, const Material *material):
        _center(center), _scale(scale), _radius(radius), _material(material)
    {
        if (!name.empty())
            this->_name = name;

        // std::cout << "Sphere constructed" << std::endl;
    }

    std::string Sphere::getName() const
    {
        return (this->_name);
    }

    std::string Sphere::getTypeName() const
    {
        return PRIMITIVE_SPHERE;
    }

    Vector3f Sphere::getPosition() const
    {
        return (this->_center);
    }

    Vector3f Sphere::getRotation() const
    {
        return (this->_rotation);
    }

    Vector3f Sphere::getScale() const
    {
        return (this->_scale);
    }

    void Sphere::setPosition(const Vector3f &position)
    {
        this->_center = position;
    }

    void Sphere::setRotation(const Vector3f &rotation)
    {
        this->_rotation = rotation;
    }

    void Sphere::setScale(const Vector3f &scale)
    {
        this->_scale = scale;
    }

    bool Sphere::intersect(const Ray &ray, float tMin, float tMax, Intersection &hit) const
    {
        float sx = (this->_scale.x == 0.0f) ? 1e-6f : this->_scale.x;
        float sy = (this->_scale.y == 0.0f) ? 1e-6f : this->_scale.y;
        float sz = (this->_scale.z == 0.0f) ? 1e-6f : this->_scale.z;

        Vector3f oc = this->_center - ray.origin;
        Vector3f local_origin(oc.x / sx, oc.y / sy, oc.z / sz);
        Vector3f local_direction(ray.direction.x / sx, ray.direction.y / sy, ray.direction.z / sz);

        auto a = local_direction.length_squared();
        auto h = dot(local_direction, local_origin);
        auto c = local_origin.length_squared() - this->_radius * this->_radius;

        auto discriminant = h * h - a * c;

        if (discriminant < 0)
            return (false);

        auto sqrtd = std::sqrt(discriminant);

        // Find the nearest root that lies in the acceptable range.
        auto root = (h - sqrtd) / a;
        if (root <= tMin || tMax <= root) {
            root = (h + sqrtd) / a;
            if (root <= tMin || tMax <= root)
                return (false);
        }

        hit.t = root;
        hit.point = ray.at(hit.t);

        Vector3f outward_normal(
            (hit.point.x - this->_center.x) / (this->_radius * sx * sx),
            (hit.point.y - this->_center.y) / (this->_radius * sy * sy),
            (hit.point.z - this->_center.z) / (this->_radius * sz * sz)
        );
        hit.set_face_normal(ray, normalize(outward_normal));

        // Spherical UV from the direction of the hit relative to the center.
        constexpr float PI = 3.14159265358979323846f;
        Vector3f dir = normalize(hit.point - this->_center);
        float phi = std::atan2(dir.z, dir.x);
        float theta = std::asin(std::max(-1.0f, std::min(1.0f, dir.y)));
        hit.uv = Vector2f(0.5f + phi / (2.0f * PI), 0.5f + theta / PI);

        // hit.color = this->_colorF;
        if (this->_material)
            hit.material = *this->_material;
        hit.primitive = this;
        return (true);
    }

    bool Sphere::isFinite() const
    {
        return (true);
    }

    AABB Sphere::bounding_box() const
    {
        Vector3f r(this->_radius * std::abs(this->_scale.x), this->_radius * std::abs(this->_scale.y), this->_radius * std::abs(this->_scale.z));
        return AABB{this->_center - r, this->_center + r};
    }

    std::map<std::string, std::pair<std::string, PropertyType>> Sphere::getProperties() const
    {
        return
        {
            {"position", {this->_center.toString(), PropertyType::VECTOR3F}},
            {"scale", {this->_scale.toString(), PropertyType::VECTOR3F}},
            {"radius", {std::to_string(this->_radius), PropertyType::FLOAT}},
        };
    }

    void Sphere::setPropertyFloat(const std::string &key, float value)
    {
        if (key == "radius")
        {
            this->_radius = value;
        }
        else
        {
            std::cerr << "Could not update " << this->_name << " (" << this->getTypeName() << ") property : " << key << " : Unknown property" << std::endl;
        }
    }

    const Material *Sphere::getMaterial() const
    {
        return (this->_material);
    }

    void Sphere::setMaterial(const Material *material)
    {
        this->_material = material;
    }

    bool Sphere::isHidden() const
    {
        return this->_hidden;
    }

    void Sphere::setHidden(bool hidden)
    {
        this->_hidden = hidden;
    }
}
