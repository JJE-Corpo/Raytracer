/*
 * ____  _ _____   ___  _   _   _   _  _         ___ _____ _  _   _   _  _
 * |_ _| _ |_   _|/ _ \| | | | /_\ | \| |   <3  | __|_   _| || | /_\ | \| |
 *  | | | |  | | | (_) | |_| |/ _ \| .` |   <3  | _|  | | | __ |/ _ \| .` |
 *  |_| |_|  |_|  \___/ \___//_/ \_\_|\_|   <3  |___| |_| |_||_/_/ \_\_|\_|
 *
 * -----------------------------------------------------------------------
 * File:    Cone.cpp
 * Who:     Titouan & Ethan
 * Date:    5/6/26
 * -----------------------------------------------------------------------
 */

#include "Cone.hpp"
#include "../../common/Intersection.hpp"
#include "../../common/Ray.hpp"
#include "../../common/Matrix.hpp"
#include "iostream"
#include "../../core/scene/builder/SceneObjectBuilder.hpp"
// source : https://www.geometrictools.com/Documentation/IntersectionLineCone.pdf

namespace rc
{
    Cone::Cone(std::string name, const Vector3f &center, const Vector3f &rotation, const Vector3f &scale, float radius, float height, const Material *material):
        _center(center), _rotation(- rotation), _radius(radius), _height(height), _material(material), _scale(scale)
    {
        // std::cout << "Cone constructed" << std::endl;
        _center.z += height;

        if (!name.empty())
            this->_name = name;
    }

    std::string Cone::getName() const
    {
        return (this->_name);
    }

    std::string Cone::getTypeName() const
    {
        return PRIMITIVE_CONE;
    }

    Vector3f Cone::getPosition() const
    {
        return (this->_center);
    }

    Vector3f Cone::getRotation() const
    {
        return (this->_rotation);
    }

    Vector3f Cone::getScale() const
    {
        return this->_scale;
    }

    void Cone::setPosition(const Vector3f &position)
    {
        this->_center = position;
    }

    void Cone::setRotation(const Vector3f &rotation)
    {
        this->_rotation = rotation;
    }

    void Cone::setScale(const Vector3f &scale)
    {
        this->_scale = scale;
    }

    bool Cone::intersect(const Ray &ray, float tMin, float tMax, Intersection &hit) const
    {
        Vector3f V = _center;
        Vector3f D = _rotation;
        D = normalize(D);
        Vector3f P = ray.origin;
        Vector3f U = ray.direction;
        Vector3f Delta = P - V;

        float eff_height = _height * this->_scale.z;
        float eff_radius = _radius * ((this->_scale.x + this->_scale.y) * 0.5f);
        float h2 = eff_height * eff_height;
        float r2 = eff_radius * eff_radius;
        float gamma2 = h2 / (h2 + r2);

        float DdU = dot(D, U);
        float DdDelta = dot(D, Delta);
        float UdU = dot(U, U);
        float UdDelta = dot(U, Delta);
        float DeltaSqr = dot(Delta, Delta);

        float a = (DdU * DdU) - gamma2 * UdU;
        float b = 2.0f * ((DdU * DdDelta) - gamma2 * UdDelta);
        float c = (DdDelta * DdDelta) - gamma2 * DeltaSqr;

        float discriminant = b * b - 4.0f * a * c;

        if (discriminant < 0.0f) {
            return false;
        }

        float sqrtD = std::sqrt(discriminant);
        float t1 = (-b - sqrtD) / (2.0f * a);
        float t2 = (-b + sqrtD) / (2.0f * a);

        if (t1 > t2) {
            std::swap(t1, t2);
        }

        float t_final = -1.0f;
        float hit_height = 0.0f;

        if (t1 >= tMin && t1 <= tMax) {
            Vector3f hit_point = P + U * t1;
            hit_height = dot(D, hit_point - V);
            if (hit_height >= 0.0f && hit_height <= _height) {
                t_final = t1;
            }
        }

        if (t_final < 0.0f && t2 >= tMin && t2 <= tMax) {
            Vector3f hit_point = P + U * t2;
            hit_height = dot(D, hit_point - V);
            if (hit_height >= 0.0f && hit_height <= _height) {
                t_final = t2;
            }
        }

        if (t_final < 0.0f) {
            return false;
        }

        hit.t = t_final;
        hit.point = P + U * t_final;

        Vector3f CP = hit.point - V;
        float tanThetaSqr = r2 / h2;
        Vector3f N = CP - D * (hit_height * (1.0f + tanThetaSqr));

        Matrix<4> rotOnly = Matrix<4>::rotation_z(this->_rotation.z * M_PI / 180.0f)
                * Matrix<4>::rotation_y(this->_rotation.y * M_PI / 180.0f)
                * Matrix<4>::rotation_x(this->_rotation.x * M_PI / 180.0f);
        float sx = (this->_scale.x == 0.0f) ? 1e-6f : this->_scale.x;
        float sy = (this->_scale.y == 0.0f) ? 1e-6f : this->_scale.y;
        float sz = (this->_scale.z == 0.0f) ? 1e-6f : this->_scale.z;
        Vector3f nScaled(N.x / sx, N.y / sy, N.z / sz);
        Vector3f nRot = rotOnly * nScaled;
        nRot = normalize(nRot);
        hit.set_face_normal(ray, nRot);
        // hit.color = this->_colorF;
        if (this->_material)
            hit.material = *this->_material;
        hit.primitive = this;

        return true;
    }

    bool Cone::isFinite() const
    {
        return (true);
    }

    AABB Cone::bounding_box() const
    {
        Vector3f D = _rotation;
        D = normalize(D);

        Vector3f C;
        // approximate scaled height along axis
        float eff_height = _height * this->_scale.z;
        C.x = _center.x + D.x * eff_height;
        C.y = _center.y + D.y * eff_height;
        C.z = _center.z + D.z * eff_height;

        Vector3f E;
        // approximate radius scaling by XY average
        float eff_radius = _radius * ((this->_scale.x + this->_scale.y) * 0.5f);
        E.x = eff_radius * std::sqrt(std::max(0.0f, 1.0f - D.x * D.x));
        E.y = eff_radius * std::sqrt(std::max(0.0f, 1.0f - D.y * D.y));
        E.z = eff_radius * std::sqrt(std::max(0.0f, 1.0f - D.z * D.z));

        Vector3f base_min;
        base_min.x = C.x - E.x;
        base_min.y = C.y - E.y;
        base_min.z = C.z - E.z;

        Vector3f base_max;
        base_max.x = C.x + E.x;
        base_max.y = C.y + E.y;
        base_max.z = C.z + E.z;

        Vector3f final_min(
            std::min(_center.x, base_min.x),
            std::min(_center.y, base_min.y),
            std::min(_center.z, base_min.z)
        );

        Vector3f final_max(
            std::max(_center.x, base_max.x),
            std::max(_center.y, base_max.y),
            std::max(_center.z, base_max.z)
        );

        float epsilon = 0.0001f;
        final_min.x -= epsilon; final_min.y -= epsilon; final_min.z -= epsilon;
        final_max.x += epsilon; final_max.y += epsilon; final_max.z += epsilon;

        return AABB{final_min, final_max};
    }

    std::map<std::string, std::pair<std::string, PropertyType>> Cone::getProperties() const
    {
        Vector3f center = this->_center;

        center.z -= this->_height;

        return
        {
            {"position", {center.toString(), PropertyType::VECTOR3F}},
            {"rotation", {(-this->_rotation).toString(), PropertyType::VECTOR3F}},
                {"scale", {this->_scale.toString(), PropertyType::VECTOR3F}},
            {"radius", {std::to_string(this->_radius), PropertyType::FLOAT}},
            {"height", {std::to_string(this->_height), PropertyType::FLOAT}},
        };
    }

    void Cone::setPropertyFloat(const std::string &key, float value)
    {
        if (key == "radius")
            this->_radius = value;
        else if (key == "height")
            this->_height = value;
        else
        {
            std::cerr << "Could not update " << this->_name << " (" << this->getTypeName() << ") property : " << key << " : Unknown property" << std::endl;
        }
    }

    const Material *Cone::getMaterial() const
    {
        return this->_material;
    }

    void Cone::setMaterial(const Material *material)
    {
        this->_material = material;
    }

    bool Cone::isHidden() const
    {
        return this->_hidden;
    }

    void Cone::setHidden(bool hidden)
    {
        this->_hidden = hidden;
    }
}
