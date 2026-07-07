#include "SceneRegister.hpp"

#include <fstream>
#include <iostream>

#include "../../common/scene/IEditablePrimitive.hpp"
#include "../../plugins/primitive/mesh/Mesh.hpp"

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

        // A mesh baked from a primitive has no backing .obj file: serialize its
        // geometry inline (object-space vertices, faces and per-vertex normals) so
        // it round-trips without any external file.
        const Mesh *inlineMesh = dynamic_cast<const Mesh *>(primitive);
        if (inlineMesh && inlineMesh->hasInlineGeometry())
        {
            object.erase("file"); // the empty "file" property is meaningless here
            json vertices = json::array();
            for (const Vector3f &v : inlineMesh->getBaseVertices())
                vertices.push_back(vector3fJson(v));
            object["vertices"] = vertices;
            json faces = json::array();
            for (const std::array<int, 3> &f : inlineMesh->getBaseFaces())
                faces.push_back(json::array({f[0], f[1], f[2]}));
            object["faces"] = faces;
            const std::vector<Vector3f> normals = inlineMesh->getVertexNormals();
            if (!normals.empty())
            {
                json normalsJson = json::array();
                for (const Vector3f &n : normals)
                    normalsJson.push_back(vector3fJson(n));
                object["normals"] = normalsJson;
            }
        }

        // An editable primitive (Mesh, Cube, ...) persists its interactive vertex
        // edits as object-space overrides re-applied on top of the base geometry.
        // Inline meshes bake edits straight into their vertices, so skip overrides.
        const IEditablePrimitive *editable = dynamic_cast<const IEditablePrimitive *>(primitive);
        if (editable && !(inlineMesh && inlineMesh->hasInlineGeometry()))
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

        // Nested objects serialize their LOCAL transform so the hierarchy round-
        // trips. Roots keep the world values getProperties() wrote above (local ==
        // world for them), preserving byte-for-byte flat-scene output.
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
        // Nested lights serialize their local transform (see primitiveJson).
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

        // Walk the top-level nodes; serializeObject recurses into group children.
        // Top-level order matches load order, so flat scenes round-trip cleanly.
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
