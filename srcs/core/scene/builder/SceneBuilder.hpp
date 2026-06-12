//
// Created by jazema on 4/21/26.
//

#ifndef SCENEBUILDER_HPP
#define SCENEBUILDER_HPP
#include <vector>

#include "../camera/Camera.hpp"
#include "../Scene.hpp"
#include "../../../common/scene/ISceneObject.hpp"
#include "../../../plugins/light/DirectionalLight.hpp"
#include "../../../plugins/light/PointLight.hpp"

namespace rc
{
    class SceneBuilder
    {
        private:
            ICamera *_camera;
            std::vector<ISceneObject *> _objects;
            std::vector<Material *> _materials;
            float _ambientCoefficient;
            float _diffuseCoefficient;
        public:
            SceneBuilder();
            
            SceneBuilder &withCamera(Camera *camera);
            SceneBuilder &withAmbientCoefficient(float ambientCoefficient);
            SceneBuilder &withDiffuseCoefficient(float diffuseCoefficient);
            SceneBuilder &withObject(ISceneObject *object);
            SceneBuilder &withObjects(std::vector<ISceneObject *> objects);
            SceneBuilder &withMaterial(Material *material);
            SceneBuilder &withMaterials(std::vector<Material *> materials);

            Scene *build();
    };
}

#endif
