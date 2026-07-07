#ifndef MESH_HPP
    #define MESH_HPP

    #include <array>
    #include <cstddef>
    #include <map>
    #include <memory>
    #include <string>
    #include <utility>
    #include <vector>

    #include "MeshTriangle.hpp"
    #include "../../../common/scene/IPrimitive.hpp"
    #include "../../../common/scene/IEditablePrimitive.hpp"
    #include "../../../common/scene/ASceneObject.hpp"
    #include "../../../common/Material.hpp"
    #include "../../../common/Vector.hpp"
    #include "../../../common/AABB.hpp"
    #include "../../../core/scene/bvh/BVHNode.hpp"
    #include "../../../core/obj/ObjParser.hpp"

namespace rc
{
    class Mesh : public ASceneObject, public IPrimitive, public IEditablePrimitive
    {
        private:
            std::string _file;
            std::string _name = "Mesh";

            Vector3f _position = {0.0f, 0.0f, 0.0f};
            Vector3f _rotation = {0.0f, 0.0f, 0.0f};
            Vector3f _scale = {1.0f, 1.0f, 1.0f};

            const Material *_material = nullptr;
            bool _hidden = false;

            // Object-space indexed geometry (survives transforms and edits).
            std::vector<Vector3f> _baseVertices;                 // unique obj vertices (overrides applied)
            Vector3f _objCenter = {0.0f, 0.0f, 0.0f};            // fixed recenter pivot
            std::vector<std::array<int, 3>> _faces;              // vertex-index triplet per face
            std::vector<std::vector<std::pair<int, int>>> _incident; // per vertex -> (face, corner)
            std::vector<Vector3f> _faceObjNormal;                // per-face geometric normal (object space)
            std::vector<std::array<Vector3f, 3>> _cornerObjNormal; // per-face-corner display normal (object space)
            std::vector<char> _faceSmooth;                       // per-face smooth-shading flag
            std::map<std::size_t, Vector3f> _overrides;          // edited vertices (object space), for save

            // World-space cache + local acceleration structure.
            std::vector<Vector3f> _worldVertices;
            std::vector<std::unique_ptr<MeshTriangle>> _triangles;
            std::unique_ptr<BVHNode> _bvh;
            AABB _bounds{{0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f}};

            void loadObj();
            // Populate the object-space topology (vertices, faces, normals) from an
            // in-memory geometry buffer instead of a .obj file. `vertexNormals` may
            // be empty (flat shading) or one normal per vertex (smooth shading).
            void buildFromGeometry(const std::vector<Vector3f> &vertices,
                const std::vector<std::array<int, 3>> &faces,
                const std::vector<Vector3f> &vertexNormals);
            void applyOverrides(const std::vector<std::pair<int, Vector3f>> &overrides);
            void buildWorldGeometry();
            void rebuildBvh();

            Vector3f objToWorld(const Vector3f &objPos) const;
            Vector3f worldToObj(const Vector3f &worldPos) const;
            Vector3f normalObjToWorld(const Vector3f &objNormal) const;
            void recomputeFaceObjNormal(int face);
            void recomputeVertexNormalObj(int vertex);

        public:
            Mesh() = default;
            Mesh(std::string name, const std::string &file, const Vector3f &position,
                const Vector3f &rotation, const Vector3f &scale, const Material *material,
                const std::vector<std::pair<int, Vector3f>> &overrides = {});
            // In-memory construction: bake a primitive straight into a mesh (verts +
            // index triples + optional per-vertex normals) with no backing .obj file.
            // Used by Scene::convertToMesh; the geometry is serialized inline instead.
            Mesh(std::string name, const std::vector<Vector3f> &vertices,
                const std::vector<std::array<int, 3>> &faces,
                const std::vector<Vector3f> &vertexNormals, const Vector3f &position,
                const Vector3f &rotation, const Vector3f &scale, const Material *material);

            bool intersect(const Ray &ray, float tMin, float tMax, Intersection &hit) const override;
            bool isFinite() const override;
            AABB bounding_box() const override;

            std::string getName() const override;
            void setName(const std::string &name) override { this->_name = name; }
            std::string getTypeName() const override;

            std::map<std::string, std::pair<std::string, PropertyType>> getProperties() const override;
            void setPropertyFloat(const std::string &key, float value) override;

            Vector3f getPosition() const override;
            Vector3f getRotation() const override;
            Vector3f getScale() const override;
            void setPosition(const Vector3f &position) override;
            void setRotation(const Vector3f &rotation) override;
            void setScale(const Vector3f &scale) override;

            const Material *getMaterial() const override;
            void setMaterial(const Material *material) override;

            bool isHidden() const override;
            void setHidden(bool hidden) override;

            // IEditablePrimitive
            std::size_t getVertexCount() const override;
            Vector3f getVertex(std::size_t index) const override;
            void setVertex(std::size_t index, const Vector3f &worldPos) override;
            void onGeometryChanged() override;

            // Serialization: object-space edits to persist as vertex_overrides.
            std::map<std::size_t, Vector3f> getVertexOverrides() const override;

            // Inline geometry (meshes with no backing .obj file, e.g. baked from a
            // primitive): the scene serializer writes these arrays instead of a
            // "file" path so the mesh round-trips without any external file.
            bool hasInlineGeometry() const;
            const std::vector<Vector3f> &getBaseVertices() const;
            const std::vector<std::array<int, 3>> &getBaseFaces() const;
            // One normal per base vertex, or empty when the mesh is flat-shaded.
            std::vector<Vector3f> getVertexNormals() const;
    };
}

#endif
