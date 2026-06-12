//
// Created by jazema on 5/10/26.
//

#ifndef ISCENEOBJECT_HPP
#define ISCENEOBJECT_HPP
#include "../Vector.hpp"

namespace rc
{
    enum class ObjectType
    {
        PRIMITIVE,
        LIGHT,
    };

    class ISceneObject
    {
        public:
            virtual ~ISceneObject() = default;

            virtual ObjectType getObjectType() const = 0;

            virtual Vector3f getPosition() const = 0;
            virtual Vector3f getRotation() const = 0;
            virtual Vector3f getScale() const = 0;
            virtual bool isHidden() const = 0;

            virtual std::string getName() const = 0;
            virtual std::string getTypeName() const = 0;

            virtual void setPosition(const Vector3f &position) = 0;
            virtual void setRotation(const Vector3f &rotation) = 0;
            virtual void setScale(const Vector3f &scale) = 0;
            virtual void setHidden(bool hidden) = 0;
    };
}

#endif
