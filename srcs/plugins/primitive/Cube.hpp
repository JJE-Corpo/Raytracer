/*
 * ____  _ _____   ___  _   _   _   _  _         ___ _____ _  _   _   _  _
 * |_ _| _ |_   _|/ _ \| | | | /_\ | \| |   <3  | __|_   _| || | /_\ | \| |
 *  | | | |  | | | (_) | |_| |/ _ \| .` |   <3  | _|  | | | __ |/ _ \| .` |
 *  |_| |_|  |_|  \___/ \___//_/ \_\_|\_|   <3  |___| |_| |_||_/_/ \_\_|\_|
 *
 * -----------------------------------------------------------------------
 * File:    Cube.hpp
 * Who:     Titouan & Ethan
 * Date:    2026-05-10
 * -----------------------------------------------------------------------
 */

#ifndef CUBE_HPP
#define CUBE_HPP
#include "../../common/Color.hpp"
#include "../../common/scene/IPrimitive.hpp"
#include "../../common/scene/IEditablePrimitive.hpp"
#include "../../common/scene/ASceneObject.hpp"
#include "../../common/Material.hpp"
#include "../../common/Vector.hpp"
#include "../../common/Matrix.hpp"

#include <cstddef>
#include <map>
#include <utility>
#include <vector>
#include <float.h>

namespace rc
{
    struct Vec4
    {
        float x;
        float y;
        float z;
        float w;

        Vec4() : x(0.0f), y(0.0f), z(0.0f), w(1.0f)
        {
        }

        Vec4(float x, float y, float z, float w) : x(x), y(y), z(z), w(w)
        {
        }

        Vec4 operator*(const Matrix<4> &m) const
        {
            float newX = m.data[0][0] * x + m.data[0][1] * y + m.data[0][2] * z + m.data[0][3] * w;
            float newY = m.data[1][0] * x + m.data[1][1] * y + m.data[1][2] * z + m.data[1][3] * w;
            float newZ = m.data[2][0] * x + m.data[2][1] * y + m.data[2][2] * z + m.data[2][3] * w;
            float newW = m.data[3][0] * x + m.data[3][1] * y + m.data[3][2] * z + m.data[3][3] * w;
            return Vec4(newX, newY, newZ, newW);
        }
    };

    struct Parallelogram
    {
        Vec4 a;
        Vec4 b;
        Vec4 c;

        Parallelogram()
        {
        }

        Parallelogram(const Vec4 &a, const Vec4 &b, const Vec4 &c) : a(a), b(b), c(c)
        {
        }

        float intersect(const Ray &ray) const
        {
            Vec4 ab = {b.x - a.x, b.y - a.y, b.z - a.z, 0};
            Vec4 ac = {c.x - a.x, c.y - a.y, c.z - a.z, 0};
            Vec4 n = {
                ab.y * ac.z - ab.z * ac.y,
                ab.z * ac.x - ab.x * ac.z,
                ab.x * ac.y - ab.y * ac.x,
                0};

            float denom = n.x * ray.direction.x + n.y * ray.direction.y + n.z * ray.direction.z;
            if (fabs(denom) < 1e-6f)
                return FLT_MAX;

            float t = ((a.x - ray.origin.x) * n.x + (a.y - ray.origin.y) * n.y + (a.z - ray.origin.z) * n.z) / denom;
            if (t < 1e-4f)
                return FLT_MAX;

            Vec4 p = {ray.origin.x + t * ray.direction.x,
                      ray.origin.y + t * ray.direction.y,
                      ray.origin.z + t * ray.direction.z, 1};
            Vec4 ap = {p.x - a.x, p.y - a.y, p.z - a.z, 0};

            float ab2 = ab.x * ab.x + ab.y * ab.y + ab.z * ab.z;
            float ac2 = ac.x * ac.x + ac.y * ac.y + ac.z * ac.z;
            float abac = ab.x * ac.x + ab.y * ac.y + ab.z * ac.z;
            float denom2 = ab2 * ac2 - abac * abac;

            if (fabs(denom2) < 1e-8f)
                return FLT_MAX;

            float d1 = ap.x * ab.x + ap.y * ab.y + ap.z * ab.z;
            float d2 = ap.x * ac.x + ap.y * ac.y + ap.z * ac.z;

            float u = (ac2 * d1 - abac * d2) / denom2;
            float v = (ab2 * d2 - abac * d1) / denom2;

            if (u < 0 || u > 1 || v < 0 || v > 1)
                return FLT_MAX;

            return t;
        }
    };

    class Cube : public ASceneObject, public IPrimitive, public IEditablePrimitive
    {
        private:
            std::string _name = "Cube";
            Vector3f _center = {0.0f, 0.0f, 0.0f};
            Vector3f _rotation = {0.0f, 0.0f, 0.0f};
            Vector3f _scale = {1.0, 1.0, 1.0};
            float _size = 1.0f;
            // ColorF _colorF;
            const Material *_material = nullptr;

            bool _hidden = false;

            // The 8 corners in OBJECT space (default +/- size/2). Editing a corner
            // stores its new object-space position here and in _cornerOverrides so
            // the deformation survives the transform and a save/reload. Rendering
            // triangulates the 8 corners, so an unedited cube is the analytic box
            // and an edited one is a free-form 8-vertex hull.
            Vector3f _corners[8];
            std::map<std::size_t, Vector3f> _cornerOverrides;

            void resetCorners();
            Matrix<4> objectToWorldMatrix() const;
            Matrix<4> worldToObjectMatrix() const;

        public:
            Cube() = default;
            Cube(std::string name, const Vector3f &center, const Vector3f &rotation, const Vector3f &scale, float size, const Material *material,
                const std::vector<std::pair<int, Vector3f>> &overrides = {});

            bool intersect(const Ray &ray, float tMin, float tMax, Intersection &hit) const override;
            bool isFinite() const override;
            AABB bounding_box() const override;

            // IEditablePrimitive
            std::size_t getVertexCount() const override;
            Vector3f getVertex(std::size_t index) const override;
            void setVertex(std::size_t index, const Vector3f &worldPos) override;
            void onGeometryChanged() override;
            std::map<std::size_t, Vector3f> getVertexOverrides() const override;

            std::string getName() const override;
            void setName(const std::string &name) override { this->_name = name; }
            std::string getTypeName() const override;
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

            std::map<std::string, std::pair<std::string, PropertyType>> getProperties() const override;
            void setPropertyFloat(const std::string &key, float value) override;
    };
}

#endif
