//
// Created by jazema on 4/22/26.
//

#ifndef ILIGHT_HPP
#define ILIGHT_HPP

#include "../Color.hpp"
#include "ISceneObject.hpp"

namespace rc
{
    enum class LightKind
    {
        POINT,
        DIRECTIONAL,
        UNKNOWN
    };

    class ILight : public virtual ISceneObject
    {
        public:
            virtual ~ILight() = default;

            ObjectType getObjectType() const override
            {
                return ObjectType::LIGHT;
            };

            virtual LightKind getKind() const = 0;

            virtual float getIntensity() const = 0;
            virtual void setIntensity(float intensity) = 0;
            virtual ColorF getColorF() const = 0;
            virtual void setColorF(const ColorF &color) = 0;

            bool isHidden() const override = 0;
            void setHidden(bool hidden) override = 0;
    };
}

#endif
