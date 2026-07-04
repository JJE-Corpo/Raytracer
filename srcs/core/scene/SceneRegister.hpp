

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
            // Dispatch one node to the right serializer (recurses into groups).
            static nlohmann::json serializeObject(ISceneObject *object);
            static nlohmann::json materialJson(const Material *material);

            static nlohmann::json serializeScene(IScene *scene);
        public:
            SceneRegister();
            ~SceneRegister();

            void saveScene(const std::string &scene_path, IScene *scene);

            /**
             * Serialize a scene into a JSON string parsable by SceneParser.
             * @param scene Scene to serialize
             * @return The scene as a JSON document
             */
            std::string toString(IScene *scene);
    };
}

#endif
