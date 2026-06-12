//
// Created by jazema on 4/21/26.
//

#include <iostream>

#include "SceneBuilder.hpp"

namespace rc
{
    SceneBuilder::SceneBuilder() : _camera(nullptr), _ambientCoefficient(0.4f), _diffuseCoefficient(0.6f)
    {
    }

    Scene *SceneBuilder::build()
    {
        Scene *result;

        result = new Scene(this->_camera, this->_ambientCoefficient, this->_diffuseCoefficient);

        for (Material *material : this->_materials)
        {
            if (!material)
                continue;
            Material *sceneMaterial = result->createMaterial(material->getName());
            if (sceneMaterial)
                *sceneMaterial = *material;
        }

        for (ISceneObject *object : this->_objects)
        {
            switch (object->getObjectType())
            {
                case ObjectType::PRIMITIVE:
                {
                    auto *primitive = dynamic_cast<IPrimitive *>(object);
                    if (primitive)
                    {
                        const Material *material = primitive->getMaterial();
                        if (material)
                        {
                            Material *sceneMaterial = result->getMaterial(material->getName());
                            if (sceneMaterial)
                                primitive->setMaterial(sceneMaterial);
                        }
                        result->addPrimitive(primitive);
                    }
                    break;
                }
                case ObjectType::LIGHT:
                    result->addLight(dynamic_cast<ILight *>(object));
                    break;
                default:
                    std::cerr << "Unknown scene object type: " << static_cast<int>(object->getObjectType()) << std::endl;
            }
        }
        return (result);
    }

    SceneBuilder &SceneBuilder::withCamera(Camera *camera)
    {
        this->_camera = camera;
        return (*this);
    }

    SceneBuilder &SceneBuilder::withAmbientCoefficient(float ambientCoefficient)
    {
        this->_ambientCoefficient = ambientCoefficient;
        return (*this);
    }

    SceneBuilder &SceneBuilder::withDiffuseCoefficient(float diffuseCoefficient)
    {
        this->_diffuseCoefficient = diffuseCoefficient;
        return (*this);
    }

    SceneBuilder &SceneBuilder::withObject(ISceneObject *object)
    {
        this->_objects.push_back(object);
        return (*this);
    }

    SceneBuilder &SceneBuilder::withObjects(std::vector<ISceneObject *> objects)
    {
        this->_objects.insert(this->_objects.end(), objects.begin(), objects.end());
        return (*this);
    }

    SceneBuilder &SceneBuilder::withMaterial(Material *material)
    {
        this->_materials.push_back(material);
        return (*this);
    }

    SceneBuilder &SceneBuilder::withMaterials(std::vector<Material *> materials)
    {
        this->_materials.insert(this->_materials.end(), materials.begin(), materials.end());
        return (*this);
    }
}
