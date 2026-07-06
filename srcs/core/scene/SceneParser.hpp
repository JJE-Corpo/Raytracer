//
// Created by jazema on 4/24/26.
//

#ifndef SCENEPARSER_HPP
#define SCENEPARSER_HPP

#include <nlohmann/json.hpp>
#include <string>
#include <vector>

#include "builder/SceneBuilder.hpp"
#include "../PluginLoader.hpp"
#include "../../common/Color.hpp"
#include "../../common/Material.hpp"

namespace rc
{
    class IScene;

    class SceneParser
    {
        private:
            std::vector<Material *> _materials;

            static Vector3f parseVector3f(const nlohmann::json &value);
            static Vector3i parseVector3i(const nlohmann::json &value);
            static Color parseColor(const nlohmann::json &value);
            static Material parseMaterial(const nlohmann::json &object);

            static Camera *parseCamera(const nlohmann::json &object);
            std::vector<ISceneObject *> parseObjects(const nlohmann::json &array);
            void parseObjectInto(const nlohmann::json &object, std::vector<ISceneObject *> &result, ISceneObject *parent);
            std::vector<Material *> parseMaterials(const nlohmann::json &array);

            nlohmann::json openConfig(const std::string &file_path);
        public:
            SceneParser();
            ~SceneParser();

            IScene *parseScene(const std::string &scene_path);
            IScene *parseScene(const nlohmann::json &config);
    };
}

#endif
