#ifndef LIGHTBUILDER_HPP
#define LIGHTBUILDER_HPP

#include <memory>

#include "../../../common/scene/ILight.hpp"
#include "../../../common/Vector.hpp"

namespace rc
{
    enum class LightType
    {
        DIRECTIONAL,
        POINT
    };

    class LightBuilder
    {
        private:
            std::string _name;
            LightType _type;
            Vector3f _position;
            Vector3f _direction;
            float _intensity = 100000.0f;
            Color _color = Color(255, 255, 255);
        public:
            LightBuilder() = default;
            ~LightBuilder() = default;

            LightBuilder &withName(const std::string &name);
            LightBuilder &withType(LightType type);
            LightBuilder &withPosition(Vector3f position);
            LightBuilder &withDirection(Vector3f direction);
            LightBuilder &withIntensity(float intensity);
            LightBuilder &withColor(Color color);

            ILight *build() const;
    };
}

#endif
