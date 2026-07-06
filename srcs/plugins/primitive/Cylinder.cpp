//
// Created by jazema on 4/29/26.
//

#include "Cylinder.hpp"

#include <iostream>

#include "../../common/Intersection.hpp"
#include "../../common/Ray.hpp"
#include "../../common/Utils.hpp"
#include "../../common/UvMapping.hpp"
#include "../../core/scene/builder/SceneObjectBuilder.hpp"

namespace rc
{
    Cylinder::Cylinder(std::string name, const Vector3f &center, const Vector3f &rotation, const Vector3f &scale, float radius, float height, const Material *material):
        _center(center), _rotation(rotation), _scale(scale), _radius(radius), _height(height), _material(material)
    {
        if (!name.empty())
            this->_name = name;
    }

    std::string Cylinder::getName() const
    {
        return (this->_name);
    }

    std::string Cylinder::getTypeName() const
    {
        return PRIMITIVE_CYLINDER;
    }

    Vector3f Cylinder::getPosition() const
    {
        return (this->_center);
    }

    Vector3f Cylinder::getRotation() const
    {
        return (this->_rotation);
    }

    Vector3f Cylinder::getScale() const
    {
        return (this->_scale);
    }

    void Cylinder::setPosition(const Vector3f &position)
    {
        this->_center = position;
    }

    void Cylinder::setRotation(const Vector3f &rotation)
    {
        this->_rotation = rotation;
    }

    void Cylinder::setScale(const Vector3f &scale)
    {
        this->_scale = scale;
    }

    bool Cylinder::intersect(const Ray &ray, float tMin, float tMax, Intersection &hit) const
    {
        Vector3f rotation_radians(
            Utils::degrees_to_radians(this->_rotation.x),
            Utils::degrees_to_radians(this->_rotation.y),
            Utils::degrees_to_radians(this->_rotation.z)
        );
        // par défaut un cylindre pointera vers le haut (quand sa rotation est 0,0,0)
        const Vector3f axis = rotate(Vector3f(0, 0, 1), rotation_radians).unit_vector();
        float eff_height = this->_height * this->_scale.z;
        float eff_radius = this->_radius * ((this->_scale.x + this->_scale.y) * 0.5f);

        Vector3f oc = this->_center - ray.origin;
        float closest_t = tMax;
        bool found_hit = false;

        float radius_sq = eff_radius * eff_radius;

        Vector3f d_perp = ray.direction - axis * dot(ray.direction, axis);
        Vector3f oc_perp = oc - axis * dot(oc, axis);

        float A = d_perp.length_squared();
        float half_B = dot(d_perp, oc_perp);
        float C = oc_perp.length_squared() - radius_sq;

        auto try_hit_body = [&](float root) {
            if (root <= tMin || root >= closest_t)
                return;

            Vector3f point = ray.at(root);
            float projection = dot(point - this->_center, axis);

            if (projection < 0.0f || projection > eff_height)
                return;

            Vector3f toPoint = point - this->_center;
            Vector3f radial = toPoint - axis * dot(toPoint, axis);
            float sx = (this->_scale.x == 0.0f) ? 1e-6f : this->_scale.x;
            float sy = (this->_scale.y == 0.0f) ? 1e-6f : this->_scale.y;
            float sz = (this->_scale.z == 0.0f) ? 1e-6f : this->_scale.z;
            Vector3f normal = normalize(Vector3f(radial.x / sx, radial.y / sy, radial.z / sz));

            hit.t = root;
            hit.point = point;
            hit.set_face_normal(ray, normal);
            hit.uv = uvmap::cylindrical(point - this->_center, axis, eff_height);
            if (this->_material)
                hit.material = *this->_material;
            hit.primitive = this;

            closest_t = root;
            found_hit = true;
        };

        float discriminant = half_B * half_B - A * C;

        if (discriminant >= 0 && A > 0.0f)
        {
            float sqrtd = std::sqrt(discriminant);
            try_hit_body((half_B - sqrtd) / A);
            try_hit_body((half_B + sqrtd) / A);
        }

        auto try_hit_cap = [&](const Vector3f& center, const Vector3f& normal) {
            float denom = dot(ray.direction, axis);
            if (std::abs(denom) < 1e-6)
                return;

            float t = dot(center - ray.origin, axis) / denom;
            if (t <= tMin || t >= closest_t)
                return;

            Vector3f point = ray.at(t);
            Vector3f to_center = point - center;

            float dist_sq = dot(to_center, to_center) -
                            std::pow(dot(to_center, axis), 2);

            if (dist_sq > radius_sq)
                return;

            hit.t = t;
            hit.point = point;
            hit.set_face_normal(ray, normal);
            hit.uv = uvmap::planar(point - center, axis, 2.0f * eff_radius);
            // hit.color = this->_colorF;
            if (this->_material)
                hit.material = *this->_material;
            hit.primitive = this;

            closest_t = t;
            found_hit = true;
        };


        try_hit_cap(this->_center, -axis);
        try_hit_cap(this->_center + axis * eff_height, axis);

        return (found_hit);
    }

    bool Cylinder::isFinite() const
    {
        return (true);
    }

    AABB Cylinder::bounding_box() const
    {
        Vector3f rotation_radians(
            Utils::degrees_to_radians(_rotation.x),
            Utils::degrees_to_radians(_rotation.y),
            Utils::degrees_to_radians(_rotation.z)
        );

        Vector3f axis = rotate(Vector3f(0, 0, 1), rotation_radians).unit_vector();
        float eff_height = _height * this->_scale.z;
        float eff_radius = _radius * ((this->_scale.x + this->_scale.y) * 0.5f);

        Vector3f bottom = _center;
        Vector3f top = _center + axis * eff_height;

        Vector3f extent(
            eff_radius * std::sqrt(1 - axis.x * axis.x),
            eff_radius * std::sqrt(1 - axis.y * axis.y),
            eff_radius * std::sqrt(1 - axis.z * axis.z)
        );

        Vector3f min(
            std::min(bottom.x, top.x) - extent.x,
            std::min(bottom.y, top.y) - extent.y,
            std::min(bottom.z, top.z) - extent.z
        );

        Vector3f max(
            std::max(bottom.x, top.x) + extent.x,
            std::max(bottom.y, top.y) + extent.y,
            std::max(bottom.z, top.z) + extent.z
        );

        return AABB{min, max};
    }

    std::map<std::string, std::pair<std::string, PropertyType>> Cylinder::getProperties() const
    {
        return
        {
            {"position", {this->_center.toString(), PropertyType::VECTOR3F}},
            {"rotation", {this->_rotation.toString(), PropertyType::VECTOR3F}},
            {"scale", {this->_scale.toString(), PropertyType::VECTOR3F}},
            {"radius", {std::to_string(this->_radius), PropertyType::FLOAT}},
            {"height", {std::to_string(this->_height), PropertyType::FLOAT}},
        };
    }

    void Cylinder::setPropertyFloat(const std::string &key, float value)
    {
        if (key == "radius")
        {
            this->_radius = value;
        }
        else if (key == "height")
        {
            this->_height = value;
        }
        else
        {
            std::cerr << "Could not update " << this->_name << " (" << this->getTypeName() << ") property : " << key << " : Unknown property" << std::endl;
        }
    }

    const Material *Cylinder::getMaterial() const
    {
        return this->_material;
    }

    void Cylinder::setMaterial(const Material *material)
    {
        this->_material = material;
    }

    bool Cylinder::isHidden() const
    {
        return this->_hidden;
    }

    void Cylinder::setHidden(bool hidden)
    {
        this->_hidden = hidden;
    }
}
