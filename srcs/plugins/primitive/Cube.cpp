/*
 * ____  _ _____   ___  _   _   _   _  _         ___ _____ _  _   _   _  _
 * |_ _| _ |_   _|/ _ \| | | | /_\ | \| |   <3  | __|_   _| || | /_\ | \| |
 *  | | | |  | | | (_) | |_| |/ _ \| .` |   <3  | _|  | | | __ |/ _ \| .` |
 *  |_| |_|  |_|  \___/ \___//_/ \_\_|\_|   <3  |___| |_| |_||_/_/ \_\_|\_|
 *
 * -----------------------------------------------------------------------
 * File:    Cube.cpp
 * Who:     Titouan & Ethan
 * Date:    2026-05-10
 * -----------------------------------------------------------------------
 */

#include "Cube.hpp"

#include <iostream>

#include "../../common/Intersection.hpp"
#include "../../common/AABB.hpp"
#include "../../common/Matrix.hpp"
#include "../../common/UvMapping.hpp"
#include "../../core/scene/builder/SceneObjectBuilder.hpp"

rc::Cube::Cube(std::string name, const Vector3f &center, const Vector3f &rotation, const Vector3f &scale, float size, const Material *material) : _center(center), _rotation(rotation), _scale(scale), _size(size), _material(material)
{
    // std::cout << "Cube constructed" << std::endl;

    if (!name.empty())
        this->_name = name;
}

std::string rc::Cube::getName() const
{
    return (this->_name);
}

std::string rc::Cube::getTypeName() const
{
    return PRIMITIVE_CUBE;
}

rc::Vector3f rc::Cube::getPosition() const
{
    return (this->_center);
}

rc::Vector3f rc::Cube::getRotation() const
{
    return (this->_rotation);
}

rc::Vector3f rc::Cube::getScale() const
{
    return (this->_scale);
}

void rc::Cube::setPosition(const Vector3f &position)
{
    this->_center = position;
}

void rc::Cube::setRotation(const Vector3f &rotation)
{
    this->_rotation = rotation;
}

void rc::Cube::setScale(const Vector3f &scale)
{
    this->_scale = scale;
}

bool rc::Cube::intersect(const Ray &ray, float tMin, float tMax, Intersection &hit) const
{
    // http://heigeas.free.fr/laure/ray_tracing/cube.html

    float s = this->_size / 2.0f;
    // // x=+s
    // Vec4 a1(this->_center.x + s, this->_center.y + s, this->_center.z - s, 1);
    // Vec4 a2(this->_center.x + s, this->_center.y + s, this->_center.z + s, 1);
    // Vec4 a3(this->_center.x + s, this->_center.y - s, this->_center.z + s, 1);
    // Vec4 a4(this->_center.x + s, this->_center.y - s, this->_center.z - s, 1);
    // // x=-s
    // Vec4 a5(this->_center.x - s, this->_center.y - s, this->_center.z - s, 1);
    // Vec4 a6(this->_center.x - s, this->_center.y + s, this->_center.z - s, 1);
    // Vec4 a7(this->_center.x - s, this->_center.y + s, this->_center.z + s, 1);
    // Vec4 a8(this->_center.x - s, this->_center.y - s, this->_center.z + s, 1);

    Vec4 a1(+s, +s, -s, 1);
    Vec4 a2(+s, +s, +s, 1);
    Vec4 a3(+s, -s, +s, 1);
    Vec4 a4(+s, -s, -s, 1);
    Vec4 a5(-s, -s, -s, 1);
    Vec4 a6(-s, +s, -s, 1);
    Vec4 a7(-s, +s, +s, 1);
    Vec4 a8(-s, -s, +s, 1);

    Matrix<4> transform = Matrix<4>::translation(this->_center.x, this->_center.y, this->_center.z)
                    * Matrix<4>::rotation_z(this->_rotation.z * M_PI / 180.0f)
                    * Matrix<4>::rotation_y(this->_rotation.y * M_PI / 180.0f)
                    * Matrix<4>::rotation_x(this->_rotation.x * M_PI / 180.0f);
    transform = transform * Matrix<4>::scaling(this->_scale.x, this->_scale.y, this->_scale.z);
    transform = transform * Matrix<4>::scaling(this->_scale.x, this->_scale.y, this->_scale.z);

    a1 = a1 * transform;
    a2 = a2 * transform;
    a3 = a3 * transform;
    a4 = a4 * transform;
    a5 = a5 * transform;
    a6 = a6 * transform;
    a7 = a7 * transform;
    a8 = a8 * transform;

    Parallelogram faces[6] = {
        Parallelogram(a1, a2, a4), // 0 : x = +s
        Parallelogram(a6, a7, a5), // 1 : x = -s
        Parallelogram(a1, a2, a6), // 2 : y = +s
        Parallelogram(a2, a7, a3), // 3 : z = +s
        Parallelogram(a3, a8, a4), // 4 : y = -s
        Parallelogram(a4, a5, a1), // 5 : z = -s
    };

    float d = FLT_MAX;
    int faceHit = -1;
    Vector3f hitPoint;

    Intersection tmp;
    float d0 = faces[0].intersect(ray);
    if (d0 != FLT_MAX)
    {
        d = d0;
        faceHit = 0;
    }

    for (int i = 1; i < 6; ++i)
    {
        float di = faces[i].intersect(ray);
        if (di < d)
        {
            d = di;
            faceHit = i;
        }
    }

    if (faceHit == -1 || d < tMin || d > tMax)
        return false;

    Matrix<4> rotOnly = Matrix<4>::rotation_z(this->_rotation.z * M_PI / 180.0f)
                    * Matrix<4>::rotation_y(this->_rotation.y * M_PI / 180.0f)
                    * Matrix<4>::rotation_x(this->_rotation.x * M_PI / 180.0f);

    Vector3f normal(0, 0, 0);
    switch (faceHit)
    {
    case 0:
        normal.x = 1;
        break; // (a1,a2,a4) → x=+s
    case 1:
        normal.x = -1;
        break; // (a6,a7,a5) → x=-s
    case 2:
        normal.y = 1;
        break; // (a1,a2,a6) → y=+s
    case 3:
        normal.z = 1;
        break; // (a2,a7,a3) → z=+s
    case 4:
        normal.y = -1;
        break; // (a3,a8,a4) → y=-s
    case 5:
        normal.z = -1;
        break; // (a4,a5,a1) → z=-s
    }

    // For non-uniform scale, transform normals by R * S^{-1}
    float sx = (this->_scale.x == 0.0f) ? 1e-6f : this->_scale.x;
    float sy = (this->_scale.y == 0.0f) ? 1e-6f : this->_scale.y;
    float sz = (this->_scale.z == 0.0f) ? 1e-6f : this->_scale.z;
    Matrix<4> normalMatrix = rotOnly * Matrix<4>::scaling(1.0f / sx, 1.0f / sy, 1.0f / sz);
    Vec4 normal4 = Vec4(normal.x, normal.y, normal.z, 0) * normalMatrix;

    hit.t = d;
    hit.point = ray.at(d); // O + d·D
    Vector3f n(normal4.x, normal4.y, normal4.z);
    n = normalize(n);
    hit.normal = n;
    float uvSize = (this->_size != 0.0f) ? this->_size : 1.0f;
    hit.uv = uvmap::planar(hit.point - this->_center, n, uvSize);
    // hit.color = this->_colorF;
    if (this->_material)
        hit.material = *this->_material;
    hit.primitive = this;
    return true;
}

void rc::Cube::getWorldCorners(Vector3f corners[8]) const
{
    const float s = this->_size / 2.0f;
    Vec4 a[8] = {
        Vec4(+s, +s, -s, 1), Vec4(+s, +s, +s, 1), Vec4(+s, -s, +s, 1), Vec4(+s, -s, -s, 1),
        Vec4(-s, -s, -s, 1), Vec4(-s, +s, -s, 1), Vec4(-s, +s, +s, 1), Vec4(-s, -s, +s, 1),
    };
    Matrix<4> transform = Matrix<4>::translation(this->_center.x, this->_center.y, this->_center.z)
                    * Matrix<4>::rotation_z(this->_rotation.z * M_PI / 180.0f)
                    * Matrix<4>::rotation_y(this->_rotation.y * M_PI / 180.0f)
                    * Matrix<4>::rotation_x(this->_rotation.x * M_PI / 180.0f);
    transform = transform * Matrix<4>::scaling(this->_scale.x, this->_scale.y, this->_scale.z);
    transform = transform * Matrix<4>::scaling(this->_scale.x, this->_scale.y, this->_scale.z);
    for (int i = 0; i < 8; ++i)
    {
        Vec4 c = a[i] * transform;
        corners[i] = Vector3f(c.x, c.y, c.z);
    }
}

bool rc::Cube::isFinite() const
{
    return (true);
}

rc::AABB rc::Cube::bounding_box() const
{
    float s = this->_size / 2.0f;

    Vec4 corners[8] = {
        Vec4(+s, +s, -s, 1),
        Vec4(+s, +s, +s, 1),
        Vec4(+s, -s, +s, 1),
        Vec4(+s, -s, -s, 1),
        Vec4(-s, -s, -s, 1),
        Vec4(-s, +s, -s, 1),
        Vec4(-s, +s, +s, 1),
        Vec4(-s, -s, +s, 1),
    };

    Matrix<4> transform = Matrix<4>::translation(this->_center.x, this->_center.y, this->_center.z)
                    * Matrix<4>::rotation_z(this->_rotation.z * M_PI / 180.0f)
                    * Matrix<4>::rotation_y(this->_rotation.y * M_PI / 180.0f)
                    * Matrix<4>::rotation_x(this->_rotation.x * M_PI / 180.0f);

    transform = transform * Matrix<4>::scaling(this->_scale.x, this->_scale.y, this->_scale.z);
    transform = transform * Matrix<4>::scaling(this->_scale.x, this->_scale.y, this->_scale.z);

    Vector3f min(FLT_MAX, FLT_MAX, FLT_MAX);
    Vector3f max(-FLT_MAX, -FLT_MAX, -FLT_MAX);

    for (int i = 0; i < 8; ++i) {
        Vec4 corner = corners[i] * transform;
        Vector3f p(corner.x, corner.y, corner.z);

        min.x = std::min(min.x, p.x);
        min.y = std::min(min.y, p.y);
        min.z = std::min(min.z, p.z);

        max.x = std::max(max.x, p.x);
        max.y = std::max(max.y, p.y);
        max.z = std::max(max.z, p.z);
    }

    return AABB{min, max};
}

const rc::Material *rc::Cube::getMaterial() const
{
    return (this->_material);
}

bool rc::Cube::isHidden() const
{
    return (this->_hidden);
}

void rc::Cube::setHidden(bool hidden)
{
    this->_hidden = hidden;
}

void rc::Cube::setMaterial(const Material *material)
{
    this->_material = material;
}

std::map<std::string, std::pair<std::string, rc::PropertyType>> rc::Cube::getProperties() const
{
    return {
        {"position", {this->_center.toString(), PropertyType::VECTOR3F}},
        {"rotation", {this->_rotation.toString(), PropertyType::VECTOR3F}},
        {"scale", {this->_scale.toString(), PropertyType::VECTOR3F}},
        {"size", {std::to_string(this->_size), PropertyType::FLOAT}},
    };
}

void rc::Cube::setPropertyFloat(const std::string &key, float value)
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
