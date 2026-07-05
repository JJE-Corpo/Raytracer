//
// Mesh: a triangulated mesh primitive loaded from a Wavefront .obj file.
//
// The geometry is loaded once (through the existing ObjParser) into an
// object-space triangle list, recentered on its own bounding-box center so
// that `position` places the mesh center (matching the other primitives'
// convention). A local BVH (the shared BVHNode) is built over the world-space
// triangles so intersection never scans them linearly. The mesh exposes a
// single global AABB to the scene BVH, and stamps its shared material onto
// every hit resolved by the local BVH.
//

#ifndef MESH_HPP
    #define MESH_HPP

    #include <memory>
    #include <string>
    #include <vector>

    #include "MeshTriangle.hpp"
    #include "../../../common/scene/IPrimitive.hpp"
    #include "../../../common/scene/ASceneObject.hpp"
    #include "../../../common/Material.hpp"
    #include "../../../common/Vector.hpp"
    #include "../../../common/AABB.hpp"
    #include "../../../core/scene/bvh/BVHNode.hpp"
    #include "../../../core/obj/ObjParser.hpp"

namespace rc
{
    class Mesh : public ASceneObject, public IPrimitive
    {
        private:
            std::string _file;
            std::string _name = "Mesh";

            Vector3f _position = {0.0f, 0.0f, 0.0f};
            Vector3f _rotation = {0.0f, 0.0f, 0.0f};
            Vector3f _scale = {1.0f, 1.0f, 1.0f};

            const Material *_material = nullptr;
            bool _hidden = false;

            // Object-space geometry, loaded once from the .obj file.
            std::vector<ObjTriangle> _objTriangles;
            Vector3f _objCenter = {0.0f, 0.0f, 0.0f};

            // World-space geometry + local acceleration structure.
            std::vector<std::unique_ptr<MeshTriangle>> _triangles;
            std::unique_ptr<BVHNode> _bvh;
            AABB _bounds{{0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f}};

            void loadObj();
            void rebuild();

        public:
            Mesh() = default;
            Mesh(std::string name, const std::string &file, const Vector3f &position,
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
    };
}

#endif
