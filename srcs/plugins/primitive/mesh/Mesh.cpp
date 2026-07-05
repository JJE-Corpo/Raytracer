//
// Mesh.cpp
//

#include "Mesh.hpp"

#include <algorithm>
#include <cmath>
#include <iostream>
#include <set>

#include "../../../common/Intersection.hpp"
#include "../../../common/Ray.hpp"
#include "../../../core/scene/builder/SceneObjectBuilder.hpp" // PRIMITIVE_MESH

namespace rc
{
    namespace
    {
        // Above this triangle count the local BVH is not rebuilt on every drag
        // frame (only on release, via onGeometryChanged) so a dense mesh stays
        // responsive -- the viewport just renders a slightly stale surface until
        // the vertex is dropped.
        constexpr std::size_t LIVE_REBUILD_MAX = 4000;
    }

    Mesh::Mesh(std::string name, const std::string &file, const Vector3f &position,
        const Vector3f &rotation, const Vector3f &scale, const Material *material,
        const std::vector<std::pair<int, Vector3f>> &overrides)
        : _file(file), _position(position), _rotation(rotation), _scale(scale), _material(material)
    {
        if (!name.empty())
            this->_name = name;
        this->loadObj();
        this->applyOverrides(overrides);
        this->buildWorldGeometry();
        this->rebuildBvh();
    }

    Vector3f Mesh::objToWorld(const Vector3f &objPos) const
    {
        Vector3f p = objPos - this->_objCenter;
        p = Vector3f(p.x * this->_scale.x, p.y * this->_scale.y, p.z * this->_scale.z);
        p = rotate(p, degToRad(this->_rotation));
        return (p + this->_position);
    }

    Vector3f Mesh::worldToObj(const Vector3f &worldPos) const
    {
        const float sx = (this->_scale.x != 0.0f) ? this->_scale.x : 1e-6f;
        const float sy = (this->_scale.y != 0.0f) ? this->_scale.y : 1e-6f;
        const float sz = (this->_scale.z != 0.0f) ? this->_scale.z : 1e-6f;
        Vector3f p = worldPos - this->_position;
        p = inverseRotate(p, degToRad(this->_rotation));
        p = Vector3f(p.x / sx, p.y / sy, p.z / sz);
        return (p + this->_objCenter);
    }

    Vector3f Mesh::normalObjToWorld(const Vector3f &objNormal) const
    {
        const float sx = (this->_scale.x != 0.0f) ? this->_scale.x : 1e-6f;
        const float sy = (this->_scale.y != 0.0f) ? this->_scale.y : 1e-6f;
        const float sz = (this->_scale.z != 0.0f) ? this->_scale.z : 1e-6f;
        Vector3f m(objNormal.x / sx, objNormal.y / sy, objNormal.z / sz);
        m = rotate(m, degToRad(this->_rotation));
        float len = length(m);
        return (len > 0.0f) ? m * (1.0f / len) : m;
    }

    void Mesh::recomputeFaceObjNormal(int face)
    {
        const std::array<int, 3> &f = this->_faces[face];
        const Vector3f &v0 = this->_baseVertices[f[0]];
        const Vector3f &v1 = this->_baseVertices[f[1]];
        const Vector3f &v2 = this->_baseVertices[f[2]];
        Vector3f n = (v1 - v0).cross(v2 - v0);
        float len = length(n);
        this->_faceObjNormal[face] = (len > 0.0f) ? n * (1.0f / len) : Vector3f(0.0f, 0.0f, 1.0f);
    }

    void Mesh::recomputeVertexNormalObj(int vertex)
    {
        Vector3f sum(0.0f, 0.0f, 0.0f);
        for (const auto &ref : this->_incident[vertex])
            sum = sum + this->_faceObjNormal[ref.first];
        float len = length(sum);
        if (len <= 0.0f)
            return;
        const Vector3f averaged = sum * (1.0f / len);
        // Smooth this vertex across every incident face: assign the averaged
        // normal to the corresponding corner and mark those faces smooth.
        for (const auto &ref : this->_incident[vertex])
        {
            this->_cornerObjNormal[ref.first][ref.second] = averaged;
            this->_faceSmooth[ref.first] = 1;
        }
    }

    void Mesh::loadObj()
    {
        this->_baseVertices.clear();
        this->_faces.clear();
        this->_incident.clear();
        this->_faceObjNormal.clear();
        this->_cornerObjNormal.clear();
        this->_faceSmooth.clear();
        this->_overrides.clear();
        this->_objCenter = {0.0f, 0.0f, 0.0f};
        if (this->_file.empty())
            return;

        std::vector<ISceneObject *> discard;
        ObjParser parser(discard);
        parser.setEmitObjects(false);
        parser.parse(this->_file);

        this->_baseVertices = parser.getVertices();
        this->_faces = parser.getFaceVertexIndices();
        const std::vector<ObjTriangle> &tris = parser.getTriangles();
        if (this->_baseVertices.empty())
            return;

        // Fixed recenter pivot from the original vertices.
        Vector3f mn = this->_baseVertices[0];
        Vector3f mx = mn;
        for (const Vector3f &v : this->_baseVertices)
        {
            mn.x = std::min(mn.x, v.x); mn.y = std::min(mn.y, v.y); mn.z = std::min(mn.z, v.z);
            mx.x = std::max(mx.x, v.x); mx.y = std::max(mx.y, v.y); mx.z = std::max(mx.z, v.z);
        }
        this->_objCenter = (mn + mx) * 0.5f;

        // Vertex -> incident (face, corner) table.
        this->_incident.assign(this->_baseVertices.size(), {});
        for (std::size_t f = 0; f < this->_faces.size(); ++f)
            for (int c = 0; c < 3; ++c)
            {
                const int vi = this->_faces[f][c];
                if (vi >= 0 && vi < static_cast<int>(this->_baseVertices.size()))
                    this->_incident[vi].push_back({static_cast<int>(f), c});
            }

        // Per-face geometric normal, per-corner display normal + smooth flag.
        this->_faceObjNormal.assign(this->_faces.size(), Vector3f(0.0f, 0.0f, 1.0f));
        this->_cornerObjNormal.assign(this->_faces.size(), {Vector3f(), Vector3f(), Vector3f()});
        this->_faceSmooth.assign(this->_faces.size(), 0);
        for (std::size_t f = 0; f < this->_faces.size(); ++f)
        {
            this->recomputeFaceObjNormal(static_cast<int>(f));
            if (f < tris.size() && tris[f].smooth)
            {
                this->_faceSmooth[f] = 1;
                this->_cornerObjNormal[f] = {tris[f].n0, tris[f].n1, tris[f].n2};
            }
            else
            {
                const Vector3f &n = this->_faceObjNormal[f];
                this->_cornerObjNormal[f] = {n, n, n};
            }
        }
    }

    void Mesh::applyOverrides(const std::vector<std::pair<int, Vector3f>> &overrides)
    {
        std::set<int> touchedVertices;
        for (const auto &entry : overrides)
        {
            const int index = entry.first;
            if (index < 0 || index >= static_cast<int>(this->_baseVertices.size()))
                continue;
            this->_baseVertices[index] = entry.second;
            this->_overrides[static_cast<std::size_t>(index)] = entry.second;
            touchedVertices.insert(index);
        }
        if (touchedVertices.empty())
            return;

        // Recompute normals in the edited region so a reloaded edit matches what
        // the interactive edit produced.
        std::set<int> touchedFaces;
        for (int v : touchedVertices)
            for (const auto &ref : this->_incident[v])
                touchedFaces.insert(ref.first);
        std::set<int> affectedVertices;
        for (int f : touchedFaces)
        {
            this->recomputeFaceObjNormal(f);
            for (int c = 0; c < 3; ++c)
                affectedVertices.insert(this->_faces[f][c]);
        }
        for (int v : affectedVertices)
            this->recomputeVertexNormalObj(v);
    }

    void Mesh::buildWorldGeometry()
    {
        this->_worldVertices.resize(this->_baseVertices.size());
        for (std::size_t i = 0; i < this->_baseVertices.size(); ++i)
            this->_worldVertices[i] = this->objToWorld(this->_baseVertices[i]);

        this->_triangles.clear();
        this->_triangles.reserve(this->_faces.size());
        for (std::size_t f = 0; f < this->_faces.size(); ++f)
        {
            const std::array<int, 3> &face = this->_faces[f];
            const Vector3f &w0 = this->_worldVertices[face[0]];
            const Vector3f &w1 = this->_worldVertices[face[1]];
            const Vector3f &w2 = this->_worldVertices[face[2]];
            if (this->_faceSmooth[f])
                this->_triangles.push_back(std::make_unique<MeshTriangle>(w0, w1, w2,
                    this->normalObjToWorld(this->_cornerObjNormal[f][0]),
                    this->normalObjToWorld(this->_cornerObjNormal[f][1]),
                    this->normalObjToWorld(this->_cornerObjNormal[f][2])));
            else
                this->_triangles.push_back(std::make_unique<MeshTriangle>(w0, w1, w2));
        }
    }

    void Mesh::rebuildBvh()
    {
        this->_bvh.reset();
        if (this->_triangles.empty())
        {
            this->_bounds = AABB{this->_position, this->_position};
            return;
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
            this->buildWorldGeometry();
            this->rebuildBvh();
        }
    }

    void Mesh::setRotation(const Vector3f &rotation)
    {
        if (rotation != this->_rotation)
        {
            this->_rotation = rotation;
            this->buildWorldGeometry();
            this->rebuildBvh();
        }
    }

    void Mesh::setScale(const Vector3f &scale)
    {
        if (scale != this->_scale)
        {
            this->_scale = scale;
            this->buildWorldGeometry();
            this->rebuildBvh();
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

    std::size_t Mesh::getVertexCount() const
    {
        return (this->_baseVertices.size());
    }

    Vector3f Mesh::getVertex(std::size_t index) const
    {
        if (index >= this->_worldVertices.size())
            return (Vector3f(0.0f, 0.0f, 0.0f));
        return (this->_worldVertices[index]);
    }

    void Mesh::setVertex(std::size_t index, const Vector3f &worldPos)
    {
        if (index >= this->_baseVertices.size())
            return;

        const Vector3f objPos = this->worldToObj(worldPos);
        this->_baseVertices[index] = objPos;
        this->_overrides[index] = objPos;
        this->_worldVertices[index] = worldPos;

        // Faces whose geometry moved (this vertex is one of their corners).
        std::set<int> movedFaces;
        for (const auto &ref : this->_incident[index])
            movedFaces.insert(ref.first);
        for (int f : movedFaces)
            this->recomputeFaceObjNormal(f);

        // Vertices whose averaged normal changed (the moved faces' corners), and
        // then every face those vertices touch (their display normals shift).
        std::set<int> affectedVertices;
        for (int f : movedFaces)
            for (int c = 0; c < 3; ++c)
                affectedVertices.insert(this->_faces[f][c]);
        for (int v : affectedVertices)
            this->recomputeVertexNormalObj(v);

        std::set<int> facesToUpdate;
        for (int v : affectedVertices)
            for (const auto &ref : this->_incident[v])
                facesToUpdate.insert(ref.first);

        for (int f : facesToUpdate)
        {
            MeshTriangle *tri = this->_triangles[f].get();
            for (int c = 0; c < 3; ++c)
                tri->setCornerVertex(c, this->_worldVertices[this->_faces[f][c]]);
            if (this->_faceSmooth[f])
                for (int c = 0; c < 3; ++c)
                    tri->setCornerNormal(c, this->normalObjToWorld(this->_cornerObjNormal[f][c]));
        }

        // Keep the mesh AABB (scene BVH leaf) covering the moved vertex during the
        // drag; the exact box is recomputed by onGeometryChanged() on release.
        this->_bounds = BoundingBoxUtils::surrounding_box(this->_bounds, AABB{worldPos, worldPos});

        if (this->_triangles.size() <= LIVE_REBUILD_MAX)
            this->rebuildBvh();
    }

    void Mesh::onGeometryChanged()
    {
        this->rebuildBvh();
    }

    const std::map<std::size_t, Vector3f> &Mesh::getVertexOverrides() const
    {
        return (this->_overrides);
    }
}
