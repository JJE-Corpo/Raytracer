//
// Created by jazema on 4/21/26.
//

#ifndef SCENE_HPP
#define SCENE_HPP
#include <memory>
#include <mutex>
#include <vector>

#include "../../common/scene/ILight.hpp"
#include "../../common/scene/IPrimitive.hpp"
#include "../../common/scene/IScene.hpp"
#include "bvh/BVHNode.hpp"

namespace rc
{
    class Scene : public IScene
    {
        private:
            ICamera *_camera;
            std::vector<IPrimitive *> _primitives;
            std::vector<IPrimitive *> _infinitePrimitives;
            std::map<std::string, Material> _materials;
            std::vector<ILight *> _lights;
            float _ambientCoefficient;
            float _diffuseCoefficient;

            std::unique_ptr<BVHNode> _bvhRoot;
            std::mutex _mutex;
        public:
            Scene(ICamera *camera, float ambientCoefficient = 0.4f, float diffuseCoefficient = 0.6f);
            ~Scene();

            void addPrimitive(IPrimitive *primitive) override;
            void addLight(ILight *light) override;
            void addDefaultPrimitive(std::string type) override;
            void addDefaultLight(std::string type) override;
            Material *getMaterial(const std::string &name) override;
            Material *createMaterial(const std::string &name) override;
            Material *createMaterial() override;
            void clear() override;

            float getAmbientCoefficient() const override;
            float getDiffuseCoefficient() const override;
            void setAmbientCoefficient(float ambientCoefficient) override;
            void setDiffuseCoefficient(float diffuseCoefficient) override;

            ICamera &getCamera() const override;
            const std::vector<IPrimitive *> &getPrimitives() const override;
            const std::vector<ILight *> &getLights() const override;
            const std::map<std::string, Material> &getMaterials() const override;

            void buildBvh() override;
            bool intersect(const Ray& ray, float tMin, float tMax, Intersection& hit) const override;
    };
}

#endif
