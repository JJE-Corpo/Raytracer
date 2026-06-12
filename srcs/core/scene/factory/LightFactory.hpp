#ifndef LIGHTFACTORY_HPP
#define LIGHTFACTORY_HPP

#include "../../../common/scene/ILight.hpp"

#include "../builder/SceneObjectBuilder.hpp"

#include "../../../plugins/light/PointLight.hpp"
#include "../../../plugins/light/DirectionalLight.hpp"

namespace rc
{
    class LightFactory
    {
        public:
            static ILight *createLight(std::string type)
            {
                if (type == LIGHT_POINT)
                {
                    return new PointLight();
                }
                else if (type == LIGHT_DIRECTIONAL)
                {
                    return new DirectionalLight();
                }
                else
                {
                    throw std::runtime_error("Unknown light type: " + type);
                }
            }
    };
}

#endif