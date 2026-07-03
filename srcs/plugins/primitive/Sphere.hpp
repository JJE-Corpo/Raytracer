//
// Created by jazema on 4/30/26.
//

#ifndef SPHERE_HPP
#define SPHERE_HPP

#include "../../common/Color.hpp"
#include "../../common/scene/IPrimitive.hpp"
#include "../../common/Material.hpp"
#include "../../common/Vector.hpp"

namespace rc
{
    class Sphere : public IPrimitive
    {
        private:
            Vector3f _center = {0.0f, 0.0f, 0.0f};
            Vector3f _rotation = {0.0f, 0.0f, 0.0f};
            Vector3f _scale = {1.0f, 1.0f, 1.0f};
            double _radius = 1.0;
            // ColorF _colorF;
            const Material *_material = nullptr;
            std::string _name = "Sphere";

            bool _hidden = false;
        public:
            Sphere() = default;
            Sphere(std::string name, const Vector3f &center, const Vector3f &scale, float radius, const Material *material);

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
