//
// MeshTriangle: a single triangle of a Mesh, usable as a BVH leaf.
//
// It stores world-space vertices plus, when the source .obj provides them,
// world-space vertex normals for smooth (barycentric) shading. Intersection
// uses the Moeller-Trumbore algorithm. Unlike the standalone Triangle
// primitive, a MeshTriangle carries no material of its own: the owning Mesh
// stamps the shared material onto the hit after the local BVH resolves it.
//

#ifndef MESHTRIANGLE_HPP
    #define MESHTRIANGLE_HPP

    #include "../../../common/scene/IPrimitive.hpp"
    #include "../../../common/scene/ASceneObject.hpp"
    #include "../../../common/Vector.hpp"

namespace rc
{
    class MeshTriangle : public ASceneObject, public IPrimitive
    {
        private:
            Vector3f _vertex0;
            Vector3f _edge1;
            Vector3f _edge2;
            Vector3f _faceNormal;

            bool _smooth = false;
            Vector3f _normal0;
            Vector3f _normal1;
            Vector3f _normal2;

        public:
            MeshTriangle() = default;
            // Flat triangle (face normal derived from the vertices).
            MeshTriangle(const Vector3f &v0, const Vector3f &v1, const Vector3f &v2);
            // Smooth triangle (per-vertex normals interpolated at the hit point).
            MeshTriangle(const Vector3f &v0, const Vector3f &v1, const Vector3f &v2,
                const Vector3f &n0, const Vector3f &n1, const Vector3f &n2);

            bool intersect(const Ray &ray, float tMin, float tMax, Intersection &hit) const override;
            bool isFinite() const override;
            AABB bounding_box() const override;

            std::string getTypeName() const override;
            std::map<std::string, std::pair<std::string, PropertyType>> getProperties() const override;
            void setPropertyFloat(const std::string &key, float value) override;

            const Material *getMaterial() const override;
            void setMaterial(const Material *material) override;
    };
}

#endif
