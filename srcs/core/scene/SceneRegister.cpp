#include "SceneRegister.hpp"

#include <fstream>
#include <iostream>

namespace rc
{
    using json = nlohmann::json;

    SceneRegister::SceneRegister()
    {
    }

    SceneRegister::~SceneRegister()
    {
    }

    json SceneRegister::vector3fJson(const Vector3f &vector)
    {
        return json::array({vector.x, vector.y, vector.z});
    }

    json SceneRegister::vector3iJson(const Vector3i &vector)
    {
        return json::array({vector.x, vector.y, vector.z});
    }

    json SceneRegister::colorJson(const Color &color)
    {
        return json::array({static_cast<int>(color.r), static_cast<int>(color.g), static_cast<int>(color.b)});
    }

    void SceneRegister::writeProperty(json &object, const std::string &name, const std::string &value, PropertyType type)
    {
        switch (type)
        {
            case PropertyType::COLOR:
                object[name] = colorJson(Color(value));
                break;
            case PropertyType::VECTOR3F:
                object[name] = vector3fJson(Vector3f(value));
                break;
            case PropertyType::VECTOR3I:
                object[name] = vector3iJson(Vector3i(value));
                break;
            case PropertyType::VECTOR2F:
            {
                Vector2f vector(value);
                object[name] = json::array({vector.x, vector.y});
                break;
            }
            case PropertyType::VECTOR2I:
            {
                Vector2i vector(value);
                object[name] = json::array({vector.x, vector.y});
                break;
            }
            case PropertyType::FLOAT:
                object[name] = std::stof(value);
                break;
            case PropertyType::INT:
                object[name] = std::stoi(value);
                break;
            case PropertyType::STRING:
                object[name] = value;
                break;
        }
    }

    json SceneRegister::cameraJson(ICamera *camera)
    {
        json object;
        Vector2i resolution = camera->getResolution();

        object["resolution"] = json::array({resolution.x, resolution.y});
        object["position"] = vector3fJson(camera->getPosition());
        object["rotation"] = vector3fJson(camera->getRotation());
        object["fieldOfView"] = camera->getFov();
        object["samplesPerPixel"] = camera->getSamplesPerPixel();
        return object;
    }

    json SceneRegister::primitiveJson(IPrimitive *primitive)
    {
        json object;
        const Material *material = primitive->getMaterial();

        object["name"] = primitive->getName();
        object["type"] = primitive->getTypeName();
        if (material)
            object["material"] = material->getName();

        for (const auto &property : primitive->getProperties())
            writeProperty(object, property.first, property.second.first, property.second.second);
        return object;
    }

    json SceneRegister::lightJson(ILight *light)
    {
        json object;

        switch (light->getKind())
        {
            case LightKind::DIRECTIONAL:
                object["name"] = light->getName();
                object["type"] = light->getTypeName();
                object["direction"] = vector3fJson(light->getRotation());
                object["color"] = colorJson(light->getColorF().toColor());
                object["intensity"] = light->getIntensity();
                break;
            case LightKind::POINT:
                object["name"] = light->getName();
                object["type"] = light->getTypeName();
                object["position"] = vector3fJson(light->getPosition());
                object["color"] = colorJson(light->getColorF().toColor());
                object["intensity"] = light->getIntensity();
                break;
            default:
                std::cerr << "Unknown light type, cannot register" << std::endl;
        }
        return object;
    }

    json SceneRegister::materialJson(const Material *material)
    {
        json object;

        object["name"] = material->getName();
        object["model"] = material->getModelName();

        for (const auto &property : material->getProperties())
            object[property.first] = property.second;

        object["base_color"] = colorJson(material->getBaseColor().toColor());
        object["specular"] = colorJson(material->getSpecular().toColor());
        return object;
    }

    json SceneRegister::serializeScene(IScene *scene)
    {
        json root;

        root["environment"] = {
            {"ambient", scene->getAmbientCoefficient()},
            {"diffuse", scene->getDiffuseCoefficient()},
        };
        root["camera"] = cameraJson(&scene->getCamera());

        root["objects"] = json::array();
        for (const auto &primitive : scene->getPrimitives())
            root["objects"].push_back(primitiveJson(primitive));
        for (const auto &light : scene->getLights())
        {
            json serializedLight = lightJson(light);

            if (!serializedLight.empty())
                root["objects"].push_back(serializedLight);
        }

        root["materials"] = json::array();
        for (const auto &entry : scene->getMaterials())
            root["materials"].push_back(materialJson(&entry.second));

        return root;
    }

    void SceneRegister::saveScene(const std::string &scene_path, IScene *scene)
    {
        std::ofstream file(scene_path);

        if (!file.is_open())
        {
            std::cerr << "Cannot write to file: " << scene_path << std::endl;
            return;
        }
        file << serializeScene(scene).dump(4) << std::endl;
    }

    std::string SceneRegister::toString(IScene *scene)
    {
        return (serializeScene(scene).dump(4));
    }
}
