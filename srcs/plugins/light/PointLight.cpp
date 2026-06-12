#include "PointLight.hpp"

#include <iostream>


rc::PointLight::PointLight(std::string name, const Vector3f &position, const float intensity, const Color color)
    : _position(position), _intensity(intensity), _colorF(ColorF::fromColor(color)), _name(name)
{
    // std::cout << "PointLight constructed" << std::endl;
}

std::string rc::PointLight::getName() const
{
    return (this->_name);
}

std::string rc::PointLight::getTypeName() const
{
    return "point_light";
}

rc::Vector3f rc::PointLight::getPosition() const
{
    return (this->_position);
}

rc::Vector3f rc::PointLight::getRotation() const
{
    return (this->_rotation);
}

rc::Vector3f rc::PointLight::getScale() const
{
    return (this->_scale);
}


void rc::PointLight::setPosition(const Vector3f &position)
{
    this->_position = position;
}

void rc::PointLight::setRotation(const Vector3f &rotation)
{
    this->_rotation = rotation;
}

void rc::PointLight::setScale(const Vector3f &scale)
{
    this->_scale = scale;
}

float rc::PointLight::getIntensity() const
{
    return (this->_intensity);
}

void rc::PointLight::setIntensity(float intensity)
{
    this->_intensity = intensity;
}

rc::ColorF rc::PointLight::getColorF() const
{
    return (this->_colorF);
}

void rc::PointLight::setColorF(const ColorF &color)
{
    this->_colorF = color;
}

rc::LightKind rc::PointLight::getKind() const
{
    return (LightKind::POINT);
}

bool rc::PointLight::isHidden() const
{
    return this->_hidden;
}

void rc::PointLight::setHidden(bool hidden)
{
    this->_hidden = hidden;
}
