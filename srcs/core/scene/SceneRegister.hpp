

#ifndef SCENEREGISTER_HPP
#define SCENEREGISTER_HPP

#include <nlohmann/json.hpp>
#include <iostream>
#include <string>

#include "../../common/Vector.hpp"
#include "../../common/Color.hpp"
#include "../../common/Material.hpp"

#include "../../common/scene/IPrimitive.hpp"
#include "../../common/scene/ILight.hpp"
#include "../../common/scene/IScene.hpp"
#include "camera/Camera.hpp"

namespace rc
{
    class SceneRegister
    {
        private:
            static nlohmann::json vector3fJson(const Vector3f &vector);
            static nlohmann::json vector3iJson(const Vector3i &vector);
            static nlohmann::json colorJson(const Color &color);
            static void writeProperty(nlohmann::json &object, const std::string &name, const std::string &value, PropertyType type);

            static nlohmann::json cameraJson(ICamera *camera);
            static nlohmann::json primitiveJson(IPrimitive *primitive);
            static nlohmann::json lightJson(ILight *light);
            static nlohmann::json groupJson(ISceneObject *group);
            static nlohmann::json serializeObject(ISceneObject *object);
            static nlohmann::json materialJson(const Material *material);
        public:
            SceneRegister();
            ~SceneRegister();

            static nlohmann::json serializeScene(IScene *scene);

            void saveScene(const std::string &scene_path, IScene *scene);

            std::string toString(IScene *scene);
    };
}

#endif
