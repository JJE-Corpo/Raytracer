#ifndef DIRECTIONALLIGHT_HPP
#define DIRECTIONALLIGHT_HPP

#include "../../common/scene/ILight.hpp"
#include "../../common/scene/ASceneObject.hpp"
#include "../../common/Vector.hpp"

namespace rc
{
    class DirectionalLight : public ASceneObject, public ILight
    {
        private:
            Vector3f _position = {0.0, 0.0, 0.0};
            Vector3f _rotation = {0.0, 0.0, 0.0};
            Vector3f _scale = {1.0, 1.0, 1.0};
            float _intensity = 1000000.0f;
            ColorF _colorF = ColorF::fromColor(Color(255, 255, 255));
            std::string _name = "Directional Light";

            bool _hidden = false;
        public:
            DirectionalLight() = default;
            DirectionalLight(std::string name, const Vector3f &direction, float intensity, Color color);
            ~DirectionalLight() override = default;

            std::string getName() const override;
            void setName(const std::string &name) override { this->_name = name; }
            std::string getTypeName() const override;
            Vector3f getPosition() const override;
            Vector3f getRotation() const override;
            Vector3f getScale() const override;
            void setPosition(const Vector3f &position) override;
            void setRotation(const Vector3f &rotation) override;
            void setScale(const Vector3f &scale) override;

            float getIntensity() const override;
            void setIntensity(float intensity) override;
            ColorF getColorF() const override;
            void setColorF(const ColorF &color) override;

            bool isHidden() const override;
            void setHidden(bool hidden) override;

            LightKind getKind() const override;
    };
}

#endif
