#include "SceneRegister.hpp"

#include <fstream>
#include <iostream>

#include "../../common/scene/IEditablePrimitive.hpp"
#include "../../common/MaterialLibrary.hpp"

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

        if (const IEditablePrimitive *editable = dynamic_cast<const IEditablePrimitive *>(primitive))
        {
            const std::map<std::size_t, Vector3f> overrides = editable->getVertexOverrides();
            if (!overrides.empty())
            {
                nlohmann::json overrideArray = nlohmann::json::array();
                for (const auto &entry : overrides)
                {
                    nlohmann::json overrideEntry;
                    overrideEntry["index"] = static_cast<int>(entry.first);
                    overrideEntry["position"] = vector3fJson(entry.second);
                    overrideArray.push_back(overrideEntry);
                }
                object["vertex_overrides"] = overrideArray;
            }
        }

        if (primitive->getParent() != nullptr)
        {
            object["position"] = vector3fJson(primitive->getLocalPosition());
            object["rotation"] = vector3fJson(primitive->getLocalRotation());
            object["scale"] = vector3fJson(primitive->getLocalScale());
        }
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
        if (light->getParent() != nullptr)
        {
            if (light->getKind() == LightKind::DIRECTIONAL)
                object["direction"] = vector3fJson(light->getLocalRotation());
            else if (light->getKind() == LightKind::POINT)
                object["position"] = vector3fJson(light->getLocalPosition());
        }
        return object;
    }

    json SceneRegister::groupJson(ISceneObject *group)
    {
        json object;

        object["name"] = group->getName();
        object["type"] = "group";
        object["position"] = vector3fJson(group->getLocalPosition());
        object["rotation"] = vector3fJson(group->getLocalRotation());
        object["scale"] = vector3fJson(group->getLocalScale());
        object["children"] = json::array();
        for (ISceneObject *child : group->getChildren())
        {
            json serialized = serializeObject(child);
            if (!serialized.empty())
                object["children"].push_back(serialized);
        }
        return object;
    }

    json SceneRegister::serializeObject(ISceneObject *object)
    {
        if (!object)
            return json();
        switch (object->getObjectType())
        {
            case ObjectType::PRIMITIVE:
            {
                IPrimitive *primitive = dynamic_cast<IPrimitive *>(object);
                return primitive ? primitiveJson(primitive) : json();
            }
            case ObjectType::LIGHT:
            {
                ILight *light = dynamic_cast<ILight *>(object);
                return light ? lightJson(light) : json();
            }
            case ObjectType::GROUP:
                return groupJson(object);
        }
        return json();
    }

    json SceneRegister::materialJson(const Material *material)
    {
        return MaterialLibrary::toJson(*material);
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
        for (ISceneObject *object : scene->getRoots())
        {
            json serialized = serializeObject(object);

            if (!serialized.empty())
                root["objects"].push_back(serialized);
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
