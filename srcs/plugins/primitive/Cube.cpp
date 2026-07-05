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

#include <algorithm>
#include <cmath>
#include <iostream>

#include "../../common/Intersection.hpp"
#include "../../common/AABB.hpp"
#include "../../common/Matrix.hpp"
#include "../../core/scene/builder/SceneObjectBuilder.hpp"

namespace
{
    // Object-space corner signs, matching the historical a1..a8 ordering.
    const int CUBE_CORNER_SIGN[8][3] = {
        {+1, +1, -1}, {+1, +1, +1}, {+1, -1, +1}, {+1, -1, -1},
        {-1, -1, -1}, {-1, +1, -1}, {-1, +1, +1}, {-1, -1, +1},
    };

    // The 6 faces as quads (corner indices), each split into 2 triangles.
    const int CUBE_FACE[6][4] = {
        {0, 1, 2, 3}, // x = +s
        {4, 5, 6, 7}, // x = -s
        {0, 1, 6, 5}, // y = +s
        {1, 2, 7, 6}, // z = +s
        {2, 3, 4, 7}, // y = -s
        {0, 3, 4, 5}, // z = -s
    };
}

rc::Cube::Cube(std::string name, const Vector3f &center, const Vector3f &rotation, const Vector3f &scale, float size, const Material *material,
    const std::vector<std::pair<int, Vector3f>> &overrides) : _center(center), _rotation(rotation), _scale(scale), _size(size), _material(material)
{
    if (!name.empty())
        this->_name = name;
    this->resetCorners();
    for (const auto &entry : overrides)
    {
        if (entry.first < 0 || entry.first >= 8)
            continue;
        this->_corners[entry.first] = entry.second;
        this->_cornerOverrides[static_cast<std::size_t>(entry.first)] = entry.second;
    }
}

void rc::Cube::resetCorners()
{
    const float s = this->_size / 2.0f;
    for (int i = 0; i < 8; ++i)
    {
        // Keep any edited corner; only refresh the untouched ones.
        if (this->_cornerOverrides.count(static_cast<std::size_t>(i)))
            continue;
        this->_corners[i] = Vector3f(CUBE_CORNER_SIGN[i][0] * s, CUBE_CORNER_SIGN[i][1] * s, CUBE_CORNER_SIGN[i][2] * s);
    }
}

rc::Matrix<4> rc::Cube::objectToWorldMatrix() const
{
    // Preserves the historical transform exactly (scale is applied twice).
    Matrix<4> transform = Matrix<4>::translation(this->_center.x, this->_center.y, this->_center.z)
                    * Matrix<4>::rotation_z(this->_rotation.z * M_PI / 180.0f)
                    * Matrix<4>::rotation_y(this->_rotation.y * M_PI / 180.0f)
                    * Matrix<4>::rotation_x(this->_rotation.x * M_PI / 180.0f);
    transform = transform * Matrix<4>::scaling(this->_scale.x, this->_scale.y, this->_scale.z);
    transform = transform * Matrix<4>::scaling(this->_scale.x, this->_scale.y, this->_scale.z);
    return transform;
}

rc::Matrix<4> rc::Cube::worldToObjectMatrix() const
{
    const float sx = (this->_scale.x == 0.0f) ? 1e-6f : this->_scale.x;
    const float sy = (this->_scale.y == 0.0f) ? 1e-6f : this->_scale.y;
    const float sz = (this->_scale.z == 0.0f) ? 1e-6f : this->_scale.z;
    // Inverse of objectToWorldMatrix: S^-1 S^-1 Rx^-1 Ry^-1 Rz^-1 T^-1.
    Matrix<4> inverse = Matrix<4>::scaling(1.0f / sx, 1.0f / sy, 1.0f / sz)
                    * Matrix<4>::scaling(1.0f / sx, 1.0f / sy, 1.0f / sz)
                    * Matrix<4>::rotation_x(-this->_rotation.x * M_PI / 180.0f)
                    * Matrix<4>::rotation_y(-this->_rotation.y * M_PI / 180.0f)
                    * Matrix<4>::rotation_z(-this->_rotation.z * M_PI / 180.0f)
                    * Matrix<4>::translation(-this->_center.x, -this->_center.y, -this->_center.z);
    return inverse;
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
    // Transform the 8 (possibly edited) corners to world space, then intersect
    // the 12 triangles that make up the 6 faces. For an unedited cube the faces
    // are planar and this matches the analytic box; an edited corner deforms the
    // adjacent faces just like a mesh vertex would.
    const Matrix<4> transform = this->objectToWorldMatrix();
    Vector3f world[8];
    for (int i = 0; i < 8; ++i)
    {
        Vec4 c = Vec4(this->_corners[i].x, this->_corners[i].y, this->_corners[i].z, 1.0f) * transform;
        world[i] = Vector3f(c.x, c.y, c.z);
    }

    float best = FLT_MAX;
    Vector3f bestNormal(0.0f, 0.0f, 0.0f);
    const float EPS = 1e-8f;

    for (int f = 0; f < 6; ++f)
    {
        // Two triangles per quad face: (q0,q1,q2) and (q0,q2,q3).
        const int corner[4] = {CUBE_FACE[f][0], CUBE_FACE[f][1], CUBE_FACE[f][2], CUBE_FACE[f][3]};
        const int tris[2][3] = {{corner[0], corner[1], corner[2]}, {corner[0], corner[2], corner[3]}};
        for (int t = 0; t < 2; ++t)
        {
            const Vector3f &v0 = world[tris[t][0]];
            const Vector3f &v1 = world[tris[t][1]];
            const Vector3f &v2 = world[tris[t][2]];
            const Vector3f edge1 = v1 - v0;
            const Vector3f edge2 = v2 - v0;
            const Vector3f h = ray.direction.cross(edge2);
            const float a = dot(edge1, h);
            if (std::fabs(a) < EPS)
                continue;
            const float invA = 1.0f / a;
            const Vector3f sVec = ray.origin - v0;
            const float u = invA * dot(sVec, h);
            if (u < 0.0f || u > 1.0f)
                continue;
            const Vector3f q = sVec.cross(edge1);
            const float v = invA * dot(ray.direction, q);
            if (v < 0.0f || u + v > 1.0f)
                continue;
            const float dist = invA * dot(edge2, q);
            if (dist <= tMin || dist >= tMax || dist >= best)
                continue;
            best = dist;
            bestNormal = edge1.cross(edge2);
        }
    }

    if (best == FLT_MAX)
        return false;

    hit.t = best;
    hit.point = ray.at(best);
    hit.set_face_normal(ray, normalize(bestNormal));
    if (this->_material)
        hit.material = *this->_material;
    hit.primitive = this;
    return true;
}

bool rc::Cube::isFinite() const
{
    return (true);
}

rc::AABB rc::Cube::bounding_box() const
{
    Matrix<4> transform = this->objectToWorldMatrix();

    Vector3f min(FLT_MAX, FLT_MAX, FLT_MAX);
    Vector3f max(-FLT_MAX, -FLT_MAX, -FLT_MAX);

    for (int i = 0; i < 8; ++i) {
        Vec4 corner = Vec4(this->_corners[i].x, this->_corners[i].y, this->_corners[i].z, 1.0f) * transform;
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

std::size_t rc::Cube::getVertexCount() const
{
    return (8);
}

rc::Vector3f rc::Cube::getVertex(std::size_t index) const
{
    if (index >= 8)
        return (Vector3f(0.0f, 0.0f, 0.0f));
    Matrix<4> transform = this->objectToWorldMatrix();
    Vec4 c = Vec4(this->_corners[index].x, this->_corners[index].y, this->_corners[index].z, 1.0f) * transform;
    return (Vector3f(c.x, c.y, c.z));
}

void rc::Cube::setVertex(std::size_t index, const Vector3f &worldPos)
{
    if (index >= 8)
        return;
    Matrix<4> inverse = this->worldToObjectMatrix();
    Vec4 obj = Vec4(worldPos.x, worldPos.y, worldPos.z, 1.0f) * inverse;
    this->_corners[index] = Vector3f(obj.x, obj.y, obj.z);
    this->_cornerOverrides[index] = this->_corners[index];
}

void rc::Cube::onGeometryChanged()
{
    // Corners are transformed per ray in intersect(); nothing to rebuild.
}

std::map<std::size_t, rc::Vector3f> rc::Cube::getVertexOverrides() const
{
    return (this->_cornerOverrides);
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
        this->resetCorners();
    }
    else
    {
        std::cerr << "Could not update " << this->_name << " (" << this->getTypeName() << ") property : " << key << " : Unknown property" << std::endl;
    }
}
