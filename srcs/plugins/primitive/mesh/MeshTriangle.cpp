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
    void MeshTriangle::recomputeEdges()
    {
        this->_edge1 = this->_vertex[1] - this->_vertex[0];
        this->_edge2 = this->_vertex[2] - this->_vertex[0];
        this->_faceNormal = this->_edge1.cross(this->_edge2).unit_vector();
    }

    MeshTriangle::MeshTriangle(const Vector3f &v0, const Vector3f &v1, const Vector3f &v2)
    {
        this->_vertex[0] = v0;
        this->_vertex[1] = v1;
        this->_vertex[2] = v2;
        this->recomputeEdges();
    }

    MeshTriangle::MeshTriangle(const Vector3f &v0, const Vector3f &v1, const Vector3f &v2,
        const Vector3f &n0, const Vector3f &n1, const Vector3f &n2)
    {
        this->_vertex[0] = v0;
        this->_vertex[1] = v1;
        this->_vertex[2] = v2;
        this->_smooth = true;
        this->_normal[0] = n0;
        this->_normal[1] = n1;
        this->_normal[2] = n2;
        this->recomputeEdges();
    }

    void MeshTriangle::setCornerVertex(int corner, const Vector3f &worldPos)
    {
        if (corner < 0 || corner > 2)
            return;
        this->_vertex[corner] = worldPos;
        this->recomputeEdges();
    }

    void MeshTriangle::setCornerNormal(int corner, const Vector3f &worldNormal)
    {
        if (corner < 0 || corner > 2)
            return;
        this->_smooth = true;
        this->_normal[corner] = worldNormal;
    }

    Vector3f MeshTriangle::cornerVertex(int corner) const
    {
        if (corner < 0 || corner > 2)
            return (this->_vertex[0]);
        return (this->_vertex[corner]);
    }

    Vector3f MeshTriangle::faceNormal() const
    {
        return (this->_faceNormal);
    }

    bool MeshTriangle::intersect(const Ray &ray, float tMin, float tMax, Intersection &hit) const
    {
        const float EPS = 1e-8f;

        Vector3f h = ray.direction.cross(this->_edge2);
        float a = dot(this->_edge1, h);

        if (std::fabs(a) < EPS)
            return (false);

        float f = 1.0f / a;
        Vector3f s = ray.origin - this->_vertex[0];
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
            Vector3f interpolated = w * this->_normal[0] + u * this->_normal[1] + v * this->_normal[2];
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
        Vector3f min(
            std::min({this->_vertex[0].x, this->_vertex[1].x, this->_vertex[2].x}),
            std::min({this->_vertex[0].y, this->_vertex[1].y, this->_vertex[2].y}),
            std::min({this->_vertex[0].z, this->_vertex[1].z, this->_vertex[2].z})
        );
        Vector3f max(
            std::max({this->_vertex[0].x, this->_vertex[1].x, this->_vertex[2].x}),
            std::max({this->_vertex[0].y, this->_vertex[1].y, this->_vertex[2].y}),
            std::max({this->_vertex[0].z, this->_vertex[1].z, this->_vertex[2].z})
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
