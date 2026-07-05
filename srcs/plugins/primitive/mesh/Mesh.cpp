//
// Mesh.cpp
//

#include "Mesh.hpp"

#include <algorithm>
#include <iostream>

#include "../../../common/Intersection.hpp"
#include "../../../common/Ray.hpp"
#include "../../../core/scene/builder/SceneObjectBuilder.hpp"

namespace rc
{
    Mesh::Mesh(std::string name, const std::string &file, const Vector3f &position,
        const Vector3f &rotation, const Vector3f &scale, const Material *material)
        : _file(file), _position(position), _rotation(rotation), _scale(scale), _material(material)
    {
        if (!name.empty())
            this->_name = name;
        this->loadObj();
        this->rebuild();
    }

    void Mesh::loadObj()
    {
        this->_objTriangles.clear();
        this->_objCenter = {0.0f, 0.0f, 0.0f};
        if (this->_file.empty())
            return;

        // Reuse the existing .obj loader in raw mode: it fills a triangle list
        // (with vertex normals when present) instead of emitting Triangle nodes.
        std::vector<ISceneObject *> discard;
        ObjParser parser(discard);
        parser.setEmitObjects(false);
        parser.parse(this->_file);
        this->_objTriangles = parser.getTriangles();
        if (this->_objTriangles.empty())
            return;

        Vector3f mn = this->_objTriangles[0].v0;
        Vector3f mx = mn;
        for (const ObjTriangle &tri : this->_objTriangles)
        {
            for (const Vector3f &v : {tri.v0, tri.v1, tri.v2})
            {
                mn.x = std::min(mn.x, v.x);
                mn.y = std::min(mn.y, v.y);
                mn.z = std::min(mn.z, v.z);
                mx.x = std::max(mx.x, v.x);
                mx.y = std::max(mx.y, v.y);
                mx.z = std::max(mx.z, v.z);
            }
        }
        this->_objCenter = (mn + mx) * 0.5f;
    }

    void Mesh::rebuild()
    {
        this->_triangles.clear();
        this->_bvh.reset();

        if (this->_objTriangles.empty())
        {
            this->_bounds = AABB{this->_position, this->_position};
            return;
        }

        const Vector3f rot = degToRad(this->_rotation);
        const float sx = this->_scale.x;
        const float sy = this->_scale.y;
        const float sz = this->_scale.z;
        const float ix = (sx != 0.0f) ? 1.0f / sx : 0.0f;
        const float iy = (sy != 0.0f) ? 1.0f / sy : 0.0f;
        const float iz = (sz != 0.0f) ? 1.0f / sz : 0.0f;

        // Vertices: recenter on the mesh center, scale, rotate, then translate.
        auto txPoint = [&](const Vector3f &v) {
            Vector3f p = v - this->_objCenter;
            p = Vector3f(p.x * sx, p.y * sy, p.z * sz);
            p = rotate(p, rot);
            return p + this->_position;
        };
        // Normals: inverse-transpose of the linear part -> R * S^-1, renormalized.
        auto txNormal = [&](const Vector3f &n) {
            Vector3f m(n.x * ix, n.y * iy, n.z * iz);
            m = rotate(m, rot);
            float len = length(m);
            return (len > 0.0f) ? m * (1.0f / len) : m;
        };

        this->_triangles.reserve(this->_objTriangles.size());
        for (const ObjTriangle &tri : this->_objTriangles)
        {
            Vector3f w0 = txPoint(tri.v0);
            Vector3f w1 = txPoint(tri.v1);
            Vector3f w2 = txPoint(tri.v2);
            if (tri.smooth)
                this->_triangles.push_back(std::make_unique<MeshTriangle>(w0, w1, w2,
                    txNormal(tri.n0), txNormal(tri.n1), txNormal(tri.n2)));
            else
                this->_triangles.push_back(std::make_unique<MeshTriangle>(w0, w1, w2));
        }

        std::vector<BVHBuildItem> items;
        items.reserve(this->_triangles.size());
        for (std::unique_ptr<MeshTriangle> &tri : this->_triangles)
        {
            AABB bounds = tri->bounding_box();
            Vector3f centroid = (bounds.min + bounds.max) * 0.5f;
            items.push_back(BVHBuildItem{tri.get(), bounds, centroid});
        }

        this->_bvh = std::make_unique<BVHNode>(items, 0, static_cast<int>(items.size()));
        this->_bounds = this->_bvh->bounding_box();
    }

    bool Mesh::intersect(const Ray &ray, float tMin, float tMax, Intersection &hit) const
    {
        if (!this->_bvh)
            return (false);
        if (!this->_bvh->intersect(ray, tMin, tMax, hit))
            return (false);
        if (this->_material)
            hit.material = *this->_material;
        hit.primitive = this;
        return (true);
    }

    bool Mesh::isFinite() const
    {
        return (true);
    }

    AABB Mesh::bounding_box() const
    {
        return (this->_bounds);
    }

    std::string Mesh::getName() const
    {
        return (this->_name);
    }

    std::string Mesh::getTypeName() const
    {
        return PRIMITIVE_MESH;
    }

    std::map<std::string, std::pair<std::string, PropertyType>> Mesh::getProperties() const
    {
        return {
            {"file", {this->_file, PropertyType::STRING}},
            {"position", {this->_position.toString(), PropertyType::VECTOR3F}},
            {"rotation", {this->_rotation.toString(), PropertyType::VECTOR3F}},
            {"scale", {this->_scale.toString(), PropertyType::VECTOR3F}},
        };
    }

    void Mesh::setPropertyFloat(const std::string &key, float value)
    {
        (void)value;
        std::cerr << "Could not update " << this->_name << " (" << this->getTypeName()
                  << ") property : " << key << " : Unknown property" << std::endl;
    }

    Vector3f Mesh::getPosition() const
    {
        return (this->_position);
    }

    Vector3f Mesh::getRotation() const
    {
        return (this->_rotation);
    }

    Vector3f Mesh::getScale() const
    {
        return (this->_scale);
    }

    void Mesh::setPosition(const Vector3f &position)
    {
        if (position != this->_position)
        {
            this->_position = position;
            this->rebuild();
        }
    }

    void Mesh::setRotation(const Vector3f &rotation)
    {
        if (rotation != this->_rotation)
        {
            this->_rotation = rotation;
            this->rebuild();
        }
    }

    void Mesh::setScale(const Vector3f &scale)
    {
        if (scale != this->_scale)
        {
            this->_scale = scale;
            this->rebuild();
        }
    }

    const Material *Mesh::getMaterial() const
    {
        return (this->_material);
    }

    void Mesh::setMaterial(const Material *material)
    {
        this->_material = material;
    }

    bool Mesh::isHidden() const
    {
        return (this->_hidden);
    }

    void Mesh::setHidden(bool hidden)
    {
        this->_hidden = hidden;
    }
}
