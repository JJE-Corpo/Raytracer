//
// Created by jazema on 4/24/26.
//

#ifndef SCENEPARSER_HPP
#define SCENEPARSER_HPP

#include <libconfig.h++>
#include <string>

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

            static Vector3f parseVector3f(const libconfig::Setting &section);
            static Vector3i parseVector3i(const libconfig::Setting &section);
            static Color parseColor(const libconfig::Setting &section);
            static Material parseMaterial(const libconfig::Setting &section);

            static Camera *parseCamera(const libconfig::Setting &section);
            std::vector<ISceneObject *> parseObjects(const libconfig::Setting &section);
            std::vector<Material *> parseMaterials(const libconfig::Setting &section);

            void openConfig(libconfig::Config &config, const std::string &file_path);
        public:
            SceneParser();
            ~SceneParser();

            IScene *parseScene(const std::string &scene_path);
            IScene *parseScene(const libconfig::Config &config);
    };
}

#endif
