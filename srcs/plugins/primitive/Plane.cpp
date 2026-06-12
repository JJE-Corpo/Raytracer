//
// Created by jazema on 4/30/26.
//

#include "Plane.hpp"

#include <iostream>

#include "../../common/Intersection.hpp"
#include "../../common/Ray.hpp"
#include "../../core/scene/builder/SceneObjectBuilder.hpp"

namespace rc
{
    Plane::Plane(std::string name, Vector3f origin, Vector3f normal, float size, const Material *material): _origin(origin), _normal(normal), _size(size), _material(material)
    {
        if (!name.empty())
            this->_name = name;
    }

    std::string Plane::getName() const
    {
        return (this->_name);
    }

    std::string Plane::getTypeName() const
    {
        return PRIMITIVE_PLANE;
    }

    Vector3f Plane::getPosition() const
    {
        return (this->_origin);
    }

    Vector3f Plane::getRotation() const
    {
        return (this->_rotation);
    }

    Vector3f Plane::getScale() const
    {
        return (this->_scale);
    }

    void Plane::setPosition(const Vector3f &position)
    {
        this->_origin = position;
    }

    void Plane::setRotation(const Vector3f &rotation)
    {
        this->_rotation = rotation;
    }

    void Plane::setScale(const Vector3f &scale)
    {
        this->_scale = scale;
    }

    bool Plane::intersect(const Ray &ray, float tMin, float tMax, Intersection &hit) const
    {
        const float EPS = 1e-6f;

        float denom = dot(this->_normal, ray.direction);

        (void)this->_size;
        if (std::fabs(denom) < EPS)
            return (false);

        float t = dot(this->_origin - ray.origin, this->_normal) / denom;
        if (t <= tMin || t >= tMax)
            return (false);

        hit.t = t;
        hit.point = ray.origin + t * ray.direction;
        hit.set_face_normal(ray, (denom < 0.0f) ? this->_normal : -this->_normal);
        // hit.color = this->_colorF;
        if (this->_material)
            hit.material = *this->_material;
        hit.primitive = this;

        return (true);
    }

    bool Plane::isFinite() const
    {
        return (false);
    }

    AABB Plane::bounding_box() const
    {
        return AABB();
    }

    std::map<std::string, std::pair<std::string, PropertyType>> Plane::getProperties() const
    {
        return
        {
            {"origin", {this->_origin.toString(), PropertyType::VECTOR3F}},
            {"normal", {this->_normal.toString(), PropertyType::VECTOR3F}},
            {"size", {std::to_string(this->_size), PropertyType::FLOAT}},
        };
    }

    void Plane::setPropertyFloat(const std::string &key, float value)
    {
        if (key == "size")
        {
            this->_size = value;
        }
        else
        {
            std::cerr << "Could not update " << this->_name << " (" << this->getTypeName() << ") property : " << key << " : Unknown property" << std::endl;
        }
    }

    const Material *Plane::getMaterial() const
    {
        return this->_material;
    }

    void Plane::setMaterial(const Material *material)
    {
        this->_material = material;
    }

    bool Plane::isHidden() const
    {
        return this->_hidden;
    }

    void Plane::setHidden(bool hidden)
    {
        this->_hidden = hidden;
    }
}
