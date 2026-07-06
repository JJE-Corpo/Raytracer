//
// MeshTriangle.cpp
//

#include "MeshTriangle.hpp"

#include <algorithm>
#include <cmath>

#include "../../../common/Intersection.hpp"
#include "../../../common/Ray.hpp"
#include "../../../common/AABB.hpp"

namespace rc
{
    MeshTriangle::MeshTriangle(const Vector3f &v0, const Vector3f &v1, const Vector3f &v2)
        : _vertex0(v0), _edge1(v1 - v0), _edge2(v2 - v0),
          _faceNormal(this->_edge1.cross(this->_edge2).unit_vector())
    {
    }

    MeshTriangle::MeshTriangle(const Vector3f &v0, const Vector3f &v1, const Vector3f &v2,
        const Vector3f &n0, const Vector3f &n1, const Vector3f &n2)
        : _vertex0(v0), _edge1(v1 - v0), _edge2(v2 - v0),
          _faceNormal(this->_edge1.cross(this->_edge2).unit_vector()),
          _smooth(true), _normal0(n0), _normal1(n1), _normal2(n2)
    {
    }

    bool MeshTriangle::intersect(const Ray &ray, float tMin, float tMax, Intersection &hit) const
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

        Vector3f normal = this->_faceNormal;
        if (this->_smooth)
        {
            float w = 1.0f - u - v;
            Vector3f interpolated = w * this->_normal0 + u * this->_normal1 + v * this->_normal2;
            float len = static_cast<float>(interpolated.length());
            if (len > 0.0f)
                normal = interpolated / len;
        }
        hit.set_face_normal(ray, normal);
        hit.uv = Vector2f(u, v);
        hit.primitive = this;
        return (true);
    }

    bool MeshTriangle::isFinite() const
    {
        return (true);
    }

    AABB MeshTriangle::bounding_box() const
    {
        Vector3f v1 = this->_vertex0 + this->_edge1;
        Vector3f v2 = this->_vertex0 + this->_edge2;
        Vector3f min(
            std::min({this->_vertex0.x, v1.x, v2.x}),
            std::min({this->_vertex0.y, v1.y, v2.y}),
            std::min({this->_vertex0.z, v1.z, v2.z})
        );
        Vector3f max(
            std::max({this->_vertex0.x, v1.x, v2.x}),
            std::max({this->_vertex0.y, v1.y, v2.y}),
            std::max({this->_vertex0.z, v1.z, v2.z})
        );
        return AABB{min, max};
    }

    std::string MeshTriangle::getTypeName() const
    {
        return ("mesh_triangle");
    }

    std::map<std::string, std::pair<std::string, PropertyType>> MeshTriangle::getProperties() const
    {
        return {};
    }

    void MeshTriangle::setPropertyFloat(const std::string &, float)
    {
    }

    const Material *MeshTriangle::getMaterial() const
    {
        return (nullptr);
    }

    void MeshTriangle::setMaterial(const Material *)
    {
    }
}
