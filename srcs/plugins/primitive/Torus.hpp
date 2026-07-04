#ifndef TORUS_HPP
#define TORUS_HPP

#include "../../common/Color.hpp"
#include "../../common/scene/IPrimitive.hpp"
#include "../../common/scene/ASceneObject.hpp"
#include "../../common/Vector.hpp"

namespace rc
{
    class Torus : public ASceneObject, public IPrimitive
    {
    private:
        Vector3f _center = {0.0f, 0.0f, 0.0f};
        Vector3f _rotation = {0.0f, 0.0f, 0.0f};
        double _radius = 1.0;
        float _height = 1.0f;
        ColorF _colorF = {0.0f, 0.0f, 0.0f};
        const Material *_material = nullptr;
        bool _hidden = false;
        std::string _name = "Torus";
        Vector3f _scale = {1.0f, 1.0f, 1.0f};
        #define EQN_EPS 1e-9
        #define IS_ZERO(x) ((x) > -EQN_EPS && (x) < EQN_EPS)

    public:
        Torus() = default;
        Torus(std::string name, const Vector3f &center, const Vector3f &rotation, float radius, float height, const Material *material);

        std::string getName() const override;
        void setName(const std::string &name) override { this->_name = name; }
        bool intersect(const Ray &ray, float tMin, float tMax, Intersection &hit) const override;
        bool isFinite() const override;
        AABB bounding_box() const override;
        std::map<std::string, std::pair<std::string, PropertyType>> getProperties() const override;

        std::string getTypeName() const override;
        Vector3f getPosition() const override;
        Vector3f getRotation() const override;
        Vector3f getScale() const override;
        void setPosition(const Vector3f &position) override;
        void setRotation(const Vector3f &rotation) override;
        void setScale(const Vector3f &scale) override;

        const Material *getMaterial() const override;
        void setMaterial(const Material *material) override;
        void setPropertyFloat(const std::string &key, float value) override;

        bool isHidden() const override;
        void setHidden(bool hidden) override;
    };
}

#endif
