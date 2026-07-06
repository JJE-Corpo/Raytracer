#include "SceneObjectBuilder.hpp"

#include <iostream>

namespace rc
{
    SceneObjectBuilder::SceneObjectBuilder() : _objectType(ObjectType::PRIMITIVE), _hasObjectType(false)
    {
    }

    SceneObjectBuilder &SceneObjectBuilder::withName(const std::string &name)
    {
        this->_primitiveBuilder.withName(name);
        this->_lightBuilder.withName(name);
        return (*this);
    }

    SceneObjectBuilder &SceneObjectBuilder::withType(const std::string &type)
    {
        this->_hasObjectType = true;
        this->_objectType = ObjectType::PRIMITIVE;
        this->_primitiveBuilder.withType(type);
        return (*this);
    }

    SceneObjectBuilder &SceneObjectBuilder::withType(LightType type)
    {
        this->_hasObjectType = true;
        this->_objectType = ObjectType::LIGHT;
        this->_lightBuilder.withType(type);
        return (*this);
    }

    SceneObjectBuilder &SceneObjectBuilder::withPosition(Vector3f position)
    {
        this->_primitiveBuilder.withPosition(position);
        this->_lightBuilder.withPosition(position);
        return (*this);
    }

    SceneObjectBuilder &SceneObjectBuilder::withRotation(Vector3f rotation)
    {
        this->_primitiveBuilder.withRotation(rotation);
        this->_lightBuilder.withDirection(rotation);
        return (*this);
    }

    SceneObjectBuilder &SceneObjectBuilder::withScale(Vector3f scale)
    {
        this->_primitiveBuilder.withScale(scale);
        return (*this);
    }

    SceneObjectBuilder &SceneObjectBuilder::withVertex0(Vector3f vertex0)
    {
        this->_primitiveBuilder.withVertex0(vertex0);
        return (*this);
    }

    SceneObjectBuilder &SceneObjectBuilder::withVertex1(Vector3f vertex1)
    {
        this->_primitiveBuilder.withVertex1(vertex1);
        return (*this);
    }

    SceneObjectBuilder &SceneObjectBuilder::withVertex2(Vector3f vertex2)
    {
        this->_primitiveBuilder.withVertex2(vertex2);
        return (*this);
    }

    SceneObjectBuilder &SceneObjectBuilder::withRadius(float radius)
    {
        this->_primitiveBuilder.withRadius(radius);
        return (*this);
    }

    SceneObjectBuilder &SceneObjectBuilder::withHeight(float height)
    {
        this->_primitiveBuilder.withHeight(height);
        return (*this);
    }

    SceneObjectBuilder &SceneObjectBuilder::withAxis(const Axis &axis)
    {
        this->_primitiveBuilder.withAxis(axis);
        return (*this);
    }

    SceneObjectBuilder &SceneObjectBuilder::withSize(float size)
    {
        this->_primitiveBuilder.withSize(size);
        return (*this);
    }

    SceneObjectBuilder &SceneObjectBuilder::withFile(const std::string &file)
    {
        this->_primitiveBuilder.withFile(file);
        return (*this);
    }

    SceneObjectBuilder &SceneObjectBuilder::withVertexOverrides(const std::vector<std::pair<int, Vector3f>> &overrides)
    {
        this->_primitiveBuilder.withVertexOverrides(overrides);
        return (*this);
    }

    SceneObjectBuilder &SceneObjectBuilder::withMaterial(const Material *material)
    {
        this->_primitiveBuilder.withMaterial(material);
        return (*this);
    }

    SceneObjectBuilder &SceneObjectBuilder::withPower(float power)
    {
        this->_primitiveBuilder.withPower(power);
        return (*this);
    }

    SceneObjectBuilder &SceneObjectBuilder::withIterations(int iterations)
    {
        this->_primitiveBuilder.withIterations(iterations);
        return (*this);
    }

    SceneObjectBuilder &SceneObjectBuilder::withThreshold(float threshold)
    {
        this->_primitiveBuilder.withThreshold(threshold);
        return (*this);
    }

    SceneObjectBuilder &SceneObjectBuilder::withDirection(Vector3f direction)
    {
        this->_lightBuilder.withDirection(direction);
        return (*this);
    }

    SceneObjectBuilder &SceneObjectBuilder::withIntensity(float intensity)
    {
        this->_lightBuilder.withIntensity(intensity);
        return (*this);
    }

    SceneObjectBuilder &SceneObjectBuilder::withColor(Color color)
    {
        this->_lightBuilder.withColor(color);
        return (*this);
    }

    ISceneObject *SceneObjectBuilder::build() const
    {
        if (!this->_hasObjectType)
            return nullptr;
        switch (this->_objectType)
        {
            case ObjectType::PRIMITIVE:
                return this->_primitiveBuilder.build();
            case ObjectType::LIGHT:
                return this->_lightBuilder.build();
            case ObjectType::GROUP:
                // Groups are constructed directly by the parser, not this builder.
                return nullptr;
        }
        return nullptr;
    }
}