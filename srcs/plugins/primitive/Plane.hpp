//
// Created by jazema on 4/30/26.
//

#ifndef PLANE_HPP
#define PLANE_HPP
#include "../../common/Color.hpp"
#include "../../common/scene/IPrimitive.hpp"
#include "../../common/Material.hpp"
#include "../../common/Vector.hpp"

namespace rc
{
    class Plane : public IPrimitive
    {
        private:
            Vector3f _origin = {0.0f, 0.0f, 0.0f};
            Vector3f _rotation = {0.0f, 0.0f, 0.0f};
            Vector3f _scale = Vector3f(1.0f, 1.0f, 1.0f);
            Vector3f _normal = {0.0f, 0.0f, 1.0f};
            float _size = 1.0f;
            // ColorF _colorF;
            const Material *_material = nullptr;
            std::string _name = "Plane";

            bool _hidden = false;
        public:
            Plane() = default;
            Plane(std::string name, Vector3f origin, Vector3f _direction, float size, const Material *material);

            bool intersect(const Ray &ray, float tMin, float tMax, Intersection &hit) const override;
            bool isFinite() const override;
            AABB bounding_box() const override;

            std::string getName() const override;
            void setName(const std::string &name) override { this->_name = name; }
            std::string getTypeName() const override;
            Vector3f getPosition() const override;
            Vector3f getRotation() const override;
            Vector3f getScale() const override;
            void setPosition(const Vector3f &position) override;
            void setRotation(const Vector3f &rotation) override;
            void setScale(const Vector3f &scale) override;

            std::map<std::string, std::pair<std::string, PropertyType>> getProperties() const override;
            void setPropertyFloat(const std::string &key, float value) override;

            const Material *getMaterial() const override;
            void setMaterial(const Material *material) override;

            bool isHidden() const override;
            void setHidden(bool hidden) override;
    };
}

#endif
