/*
** EPITECH PROJECT, 2025
** raytracer [WSL: Ubuntu-22.04]
** File description:
** Triangle.cpp
*/

#include "Triangle.hpp"
#include <cmath>
#include <iostream>
#include "../../common/Intersection.hpp"
#include "../../common/Ray.hpp"
#include "../../common/AABB.hpp"
#include "../../common/Material.hpp"
#include "../../core/scene/builder/SceneObjectBuilder.hpp"

namespace rc
{
    Triangle::Triangle(std::string name, const Vector3f &v0, const Vector3f &v1, const Vector3f &v2, const Material *material) :
        _vertex0(v0), _vertex1(v1), _vertex2(v2), _edge1(v1 - v0), _edge2(v2 - v0), _normal(this->_edge1.cross(this->_edge2).unit_vector()), _material(material)
    {
        if (!name.empty())
            this->_name = name;

        // std::cout << "Triangle constructed" << std::endl;
    }

    bool Triangle::intersect(const Ray &ray, float tMin, float tMax, Intersection &hit) const
    {
        const float EPS = 1e-8f;

        Vector3f h = ray.direction.cross(this->_edge2);
        float a = dot(this->_edge1, h);

        if (std::fabs(a) < EPS)
            return (false);

        float f = 1.0f / a;
        Vector3f s = ray.origin - this->_vertex0;
        float u = f * dot(s, h);

        if (u < 0.0f || u > 1.0f)
            return (false);

        Vector3f q = s.cross(this->_edge1);
        float v = f * dot(ray.direction, q);

        if (v < 0.0f || u + v > 1.0f)
            return (false);

        float t = f * dot(this->_edge2, q);

        if (t <= tMin || tMax <= t)
            return (false);

        hit.t = t;
        hit.point = ray.at(hit.t);
        hit.set_face_normal(ray, this->_normal);
        // hit.color = this->_colorF;
        if (this->_material)
            hit.material = *this->_material;
        hit.primitive = this;
        return (true);
    }

    bool Triangle::isFinite() const
    {
        return (true);
    }

    AABB Triangle::bounding_box() const
    {
        Vector3f min(
            std::min({this->_vertex0.x, this->_vertex1.x, this->_vertex2.x}),
            std::min({this->_vertex0.y, this->_vertex1.y, this->_vertex2.y}),
            std::min({this->_vertex0.z, this->_vertex1.z, this->_vertex2.z})
        );
        Vector3f max(
            std::max({this->_vertex0.x, this->_vertex1.x, this->_vertex2.x}),
            std::max({this->_vertex0.y, this->_vertex1.y, this->_vertex2.y}),
            std::max({this->_vertex0.z, this->_vertex1.z, this->_vertex2.z})
        );
        return AABB{min, max};
    }

    std::string Triangle::getName() const
    {
        return (this->_name);
    }

    std::string Triangle::getTypeName() const
    {
        return PRIMITIVE_TRIANGLE;
    }

    std::map<std::string, std::pair<std::string, PropertyType>> Triangle::getProperties() const
    {
        return {
            {"vertex0", {this->_vertex0.toString(), PropertyType::VECTOR3F}},
            {"vertex1", {this->_vertex1.toString(), PropertyType::VECTOR3F}},
            {"vertex2", {this->_vertex2.toString(), PropertyType::VECTOR3F}},
        };
    }

    void Triangle::setPropertyFloat(const std::string &key, float value)
    {
        (void)value;
        std::cerr << "Could not update " << this->_name << " (" << this->getTypeName() << ") property : " << key << " : Unknown property" << std::endl;
    }

    Vector3f Triangle::getPosition() const
    {
        return Vector3f((this->_vertex0.x + this->_vertex1.x + this->_vertex2.x) / 3.0f,
                        (this->_vertex0.y + this->_vertex1.y + this->_vertex2.y) / 3.0f,
                        (this->_vertex0.z + this->_vertex1.z + this->_vertex2.z) / 3.0f);
    }

    Vector3f Triangle::getRotation() const
    {
        return this->_rotation;
    }

    Vector3f Triangle::getScale() const
    {
        return this->_scale;
    }

    void Triangle::setPosition(const Vector3f &position)
    {
        Vector3f centroid = this->getPosition();
        Vector3f delta = position - centroid;
        this->_vertex0 = this->_vertex0 + delta;
        this->_vertex1 = this->_vertex1 + delta;
        this->_vertex2 = this->_vertex2 + delta;
        this->_edge1 = this->_vertex1 - this->_vertex0;
        this->_edge2 = this->_vertex2 - this->_vertex0;
        this->_normal = this->_edge1.cross(this->_edge2).unit_vector();
    }

    void Triangle::setRotation(const Vector3f &rotation)
    {
        (void)rotation;
        this->_rotation = rotation;
    }

    void Triangle::setScale(const Vector3f &scale)
    {
        (void)scale;
        this->_scale = scale;
    }

    const Material *Triangle::getMaterial() const
    {
        return this->_material;
    }

    void Triangle::setMaterial(const Material *material)
    {
        this->_material = material;
    }


    bool Triangle::isHidden() const
    {
        return this->_hidden;
    }

    void Triangle::setHidden(bool hidden)
    {
        this->_hidden = hidden;
    }
}