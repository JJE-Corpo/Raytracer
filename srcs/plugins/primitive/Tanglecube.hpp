/*
* ____  _ _____   ___  _   _   _   _  _         ___ _____ _  _   _   _  _
* |_ _| _ |_   _|/ _ \| | | | /_\ | \| |   <3  | __|_   _| || | /_\ | \| |
*  | | | |  | | | (_) | |_| |/ _ \| .` |   <3  | _|  | | | __ |/ _ \| .` |
*  |_| |_|  |_|  \___/ \___//_/ \_\_|\_|   <3  |___| |_| |_||_/_/ \_\_|\_|
*
* -----------------------------------------------------------------------
* File:    Tanglecube.hpp
* Who:     Titouan & Ethan
* Date:    2026-05-08
* -----------------------------------------------------------------------
*/


#ifndef TANGLECUBE_HPP
#define TANGLECUBE_HPP

#include "../../common/Color.hpp"
#include "../../common/scene/IPrimitive.hpp"
#include "../../common/Material.hpp"
#include "../../common/Vector.hpp"

namespace rc
{
    class Tanglecube : public IPrimitive
    {
        private:
            Vector3f _center = {0.0f, 0.0f, 0.0f};
            Vector3f _rotation = {0.0f, 0.0f, 0.0f};
            Vector3f _scale = {1.0, 1.0, 1.0};
            float _threshold = 11.8f;
            float _size = 1.0f;
            float _invSize = 1.0f;
            // ColorF _colorF;
            const Material *_material = nullptr;
            std::string _name = "Tanglecube";

            bool _hidden = false;
        public:
            Tanglecube() = default;
            Tanglecube(std::string name, Vector3f center, Vector3f rotation, float threshold, float size, const Material *material);

            bool intersect(const Ray &ray, float tMin, float tMax, Intersection &hit) const override;
            bool isFinite() const override;
            AABB bounding_box() const override;

            std::string getName() const override;
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
            // void setMaterialPropertyFloat(const std::string &key, float value) override;
    };
}

#endif
