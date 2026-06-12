//
// Created by jazema on 4/21/26.
//

#include "Scene.hpp"
#include "../../common/Intersection.hpp"

#include "factory/PrimitiveFactory.hpp"
#include "factory/LightFactory.hpp"

namespace rc
{
    Scene::Scene(ICamera *camera, float ambientCoefficient, float diffuseCoefficient): _camera(camera), _ambientCoefficient(ambientCoefficient), _diffuseCoefficient(diffuseCoefficient)
    {
    }

    Scene::~Scene()
    {
        this->Scene::clear();
        this->_mutex.lock();
        delete this->_camera;
        this->_mutex.unlock();
    }

    void Scene::addPrimitive(IPrimitive *primitive)
    {
        this->_mutex.lock();
        this->_primitives.push_back(primitive);
        this->_mutex.unlock();
    }

    void Scene::addLight(ILight *light)
    {
        this->_mutex.lock();
        this->_lights.push_back(light);
        this->_mutex.unlock();
    }

    void Scene::addDefaultPrimitive(std::string type)
    {
        IPrimitive *primitive = PrimitiveFactory::createPrimitive(type);
        if (primitive)
            addPrimitive(primitive);
    }

    void Scene::addDefaultLight(std::string type)
    {
        ILight *light = LightFactory::createLight(type);
        if (light)
            addLight(light);
    }

    Material *Scene::getMaterial(const std::string &name)
    {
        if (this->_materials.find(name) != this->_materials.end())
            return (&this->_materials[name]);
        return (nullptr);
    }

    Material *Scene::createMaterial(const std::string &name)
    {
        if (this->_materials.find(name) != this->_materials.end())
            return (&this->_materials[name]);
        Material result;

        result.name = name;
        this->_materials.emplace(name, result);
        return (&this->_materials[name]);
    }

    Material *Scene::createMaterial()
    {
        std::string name = "material #0";
        int tmp = 0;

        while (this->_materials.find(name) != this->_materials.end())
        {
            tmp++;
            name = "material #" + std::to_string(tmp);
        }
        return (createMaterial(name));
    }

    void Scene::clear()
    {
        this->_mutex.lock();
        while (!this->_primitives.empty())
        {
            delete this->_primitives.back();
            this->_primitives.pop_back();
        }
        while (!this->_lights.empty())
        {
            delete this->_lights.back();
            this->_lights.pop_back();
        }
        this->_materials.clear();
        this->_mutex.unlock();
    }

    float Scene::getAmbientCoefficient() const
    {
        return (this->_ambientCoefficient);
    }

    float Scene::getDiffuseCoefficient() const
    {
        return (this->_diffuseCoefficient);
    }

    void Scene::setAmbientCoefficient(float ambientCoefficient)
    {
        this->_mutex.lock();
        this->_ambientCoefficient = ambientCoefficient;
        this->_mutex.unlock();
    }

    void Scene::setDiffuseCoefficient(float diffuseCoefficient)
    {
        this->_mutex.lock();
        this->_diffuseCoefficient = diffuseCoefficient;
        this->_mutex.unlock();
    }

    void Scene::buildBvh()
    {
        this->_mutex.lock();
        std::vector<BVHBuildItem> finitePrimitives;

        this->_infinitePrimitives.clear();
        for (auto *primitive : this->_primitives)
        {
            if (!primitive)
                continue;
            if (primitive->isHidden())
                continue;
            if (primitive->isFinite())
            {
                const AABB bounds = primitive->bounding_box();
                finitePrimitives.push_back({primitive, bounds, (bounds.min + bounds.max) * 0.5f});
            }
            else
                this->_infinitePrimitives.push_back(primitive);
        }
        this->_bvhRoot = std::make_unique<BVHNode>(finitePrimitives, 0, finitePrimitives.size());
        this->_mutex.unlock();
    }

    ICamera &Scene::getCamera() const
    {
        return (*this->_camera);
    }

    const std::vector<IPrimitive *> &Scene::getPrimitives() const
    {
        return (this->_primitives);
    }

    const std::vector<ILight *> &Scene::getLights() const
    {
        return (this->_lights);
    }

    const std::map<std::string, Material> &Scene::getMaterials() const
    {
        return (this->_materials);
    }

    bool Scene::intersect(const Ray &ray, float tMin, float tMax, Intersection &hit) const
    {
        bool hitAnything = false;
        float closest = tMax;

        if (this->_bvhRoot)
        {
            Intersection temp;
            if (this->_bvhRoot->intersect(ray, tMin, closest, temp))
            {
                hitAnything = true;
                closest = temp.t;
                hit = temp;
            }
        }
        for (auto* obj : this->_infinitePrimitives)
        {
            if (!obj)
                continue;
            if (obj->isHidden())
                continue;
            Intersection temp;
            if (obj->intersect(ray, tMin, closest, temp))
            {
                hitAnything = true;
                closest = temp.t;
                hit = temp;
            }
        }
        return (hitAnything);
    }
}
