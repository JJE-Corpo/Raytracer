#include "DirectionalLight.hpp"

#include <iostream>


rc::DirectionalLight::DirectionalLight(std::string name, const Vector3f &direction, const float intensity, const Color color) : _rotation(direction), _intensity(intensity), _colorF(ColorF::fromColor(color)), _name(name)
{
    // std::cout << "DirectionalLight constructed" << std::endl;
}

rc::Vector3f rc::DirectionalLight::getPosition() const
{
    return (this->_position);
}

rc::Vector3f rc::DirectionalLight::getRotation() const
{
    return (this->_rotation);
}

rc::Vector3f rc::DirectionalLight::getScale() const
{
    return (this->_scale);
}

std::string rc::DirectionalLight::getName() const
{
    return (this->_name);
}

std::string rc::DirectionalLight::getTypeName() const
{
    return "directional_light";
}

void rc::DirectionalLight::setPosition(const Vector3f &position)
{
    this->_position = position;
}

void rc::DirectionalLight::setRotation(const Vector3f &rotation)
{
    this->_rotation = rotation;
}

void rc::DirectionalLight::setScale(const Vector3f &scale)
{
    this->_scale = scale;
}

float rc::DirectionalLight::getIntensity() const
{
    return (this->_intensity);
}

void rc::DirectionalLight::setIntensity(float intensity)
{
    this->_intensity = intensity;
}

rc::ColorF rc::DirectionalLight::getColorF() const
{
    return (this->_colorF);
}

void rc::DirectionalLight::setColorF(const ColorF &color)
{
    this->_colorF = color;
}

rc::LightKind rc::DirectionalLight::getKind() const
{
    return (LightKind::DIRECTIONAL);
}

bool rc::DirectionalLight::isHidden() const
{
    return (this->_hidden);
}

void rc::DirectionalLight::setHidden(bool hidden)
{
    this->_hidden = hidden;
}
