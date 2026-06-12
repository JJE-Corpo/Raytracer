

#ifndef SCENEREGISTER_HPP
#define SCENEREGISTER_HPP

#include <libconfig.h++>
#include <iostream>

#include "../../common/Vector.hpp"
#include "../../common/Color.hpp"

#include "../../common/scene/IPrimitive.hpp"
#include "../../common/scene/ILight.hpp"
#include "../../common/scene/IScene.hpp"
#include "camera/Camera.hpp"

namespace rc
{
    class SceneRegister
    {
        private:
            libconfig::Config _cfg;

            static void registerVector3f(std::string name, Vector3f vector, libconfig::Setting &setting);
            static void registerVector3i(std::string name, Vector3i vector, libconfig::Setting &setting);
            
            static void registerVector2i(std::string name, Vector2i vector, libconfig::Setting &setting);
            static void registerVector2f(std::string name, Vector2f vector, libconfig::Setting &setting);
            
            static void registerColor(std::string name, Color color, libconfig::Setting &setting);

            static void registerInt(std::string name, int value, libconfig::Setting &setting);
            static void registerFloat(std::string name, float value, libconfig::Setting &setting);
            static void registerString(const std::string& name, const std::string& value, libconfig::Setting &setting);

            void registerCamera(ICamera *camera);
            void registerPrimitive(IPrimitive *primitive);
            void registerLight(ILight *light);
            void registerMaterial(const Material *material);

            void saveToFile(const std::string &file_path);

            libconfig::Setting &getRoot();

            void serializeScene(IScene *scene);
        public:
            SceneRegister();
            ~SceneRegister();

            void saveScene(const std::string &scene_path, IScene *scene);

            /**
             * fonction de con permettant de transformer une scene en string parsable par notre lib
             * @param scene Scene a recuperer
             * @return La scene au format lisible par la config++
             */
            std::string toString(IScene *scene);
    };
}

#endif