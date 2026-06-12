#include "LightBuilder.hpp"

#include "../../../plugins/light/DirectionalLight.hpp"
#include "../../../plugins/light/PointLight.hpp"

rc::LightBuilder &rc::LightBuilder::withName(const std::string &name)
{
    this->_name = name;
    return (*this);
}

rc::LightBuilder &rc::LightBuilder::withType(const LightType type)
{
    this->_type = type;
    return (*this);
}

rc::LightBuilder &rc::LightBuilder::withPosition(const Vector3f position)
{
    this->_position = position;
    return (*this);
}

rc::LightBuilder &rc::LightBuilder::withDirection(const Vector3f direction)
{
    this->_direction = direction;
    return (*this);
}

rc::LightBuilder &rc::LightBuilder::withIntensity(const float intensity)
{
    this->_intensity = intensity;
    return (*this);
}

rc::LightBuilder &rc::LightBuilder::withColor(const Color color)
{
    this->_color = color;
    return (*this);
}

rc::ILight *rc::LightBuilder::build() const
{
    switch (this->_type)
    {
        case LightType::DIRECTIONAL:
            return (new DirectionalLight(this->_name, this->_direction, this->_intensity, this->_color));
        case LightType::POINT:
            return (new PointLight(this->_name, this->_position, this->_intensity, this->_color));
    }
    return (nullptr);
}
