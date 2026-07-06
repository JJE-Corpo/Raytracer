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
            Vector3f _vertex[3];
            Vector3f _edge1;
            Vector3f _edge2;
            Vector3f _faceNormal;

            bool _smooth = false;
            Vector3f _normal[3];

            void recomputeEdges();

        public:
            MeshTriangle() = default;
            // Flat triangle (face normal derived from the vertices).
            MeshTriangle(const Vector3f &v0, const Vector3f &v1, const Vector3f &v2);
            // Smooth triangle (per-vertex normals interpolated at the hit point).
            MeshTriangle(const Vector3f &v0, const Vector3f &v1, const Vector3f &v2,
                const Vector3f &n0, const Vector3f &n1, const Vector3f &n2);

            // --- Vertex editing ------------------------------------------------
            void setCornerVertex(int corner, const Vector3f &worldPos);
            void setCornerNormal(int corner, const Vector3f &worldNormal);
            Vector3f cornerVertex(int corner) const;
            Vector3f faceNormal() const;

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
