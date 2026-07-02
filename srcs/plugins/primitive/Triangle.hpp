/*
** EPITECH PROJECT, 2025
** raytracer [WSL: Ubuntu-22.04]
** File description:
** Triangle.hpp
*/

#ifndef TRIANGLE_HPP
    #define TRIANGLE_HPP

    #include "../../common/Color.hpp"
    #include "../../common/scene/IPrimitive.hpp"
    #include "../../common/Material.hpp"
    #include "../../common/Vector.hpp"
    #include <string>

namespace rc
{
    class Triangle : public IPrimitive
    {
        private:
            Vector3f _vertex0 = {0.0f, 0.0f, 0.0f};
            Vector3f _vertex1 = {1.0f, 0.0f, 0.0f};
            Vector3f _vertex2 = {0.0f, 1.0f, 0.0f};
            Vector3f _edge1 = {1.0f, 0.0f, 0.0f};
            Vector3f _edge2 = {0.0f, 1.0f, 0.0f};
            Vector3f _normal = {0.0f, 0.0f, 1.0f};
            // ColorF _colorF;
            const Material *_material = nullptr;
            std::string _name = "Triangle";

            Vector3f _rotation = {0.0f, 0.0f, 0.0f};
            Vector3f _scale = {1.0f, 1.0f, 1.0f};

            bool _hidden = false;
        public: 
            Triangle() = default;
            Triangle(std::string name, const Vector3f &v0, const Vector3f &v1, const Vector3f &v2, const Material *material);

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