//
// Created by jazema on 4/24/26.
//

#include "SceneParser.hpp"

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <functional>
#include <iostream>
#include <utility>
#include <vector>

#include "Scene.hpp"
#include "../../common/Color.hpp"
#include "builder/SceneBuilder.hpp"
#include "../exceptions/LoadingSceneException.hpp"
#include "builder/LightBuilder.hpp"
#include "builder/PrimitiveBuilder.hpp"
#include "builder/SceneObjectBuilder.hpp"
#include "../../core/PluginLoader.hpp"
#include "../../common/Material.hpp"
#include "../../common/MaterialLibrary.hpp"
#include "../obj/ObjParser.hpp"
#include "../../common/scene/ISceneObject.hpp"

namespace rc
{
    using json = nlohmann::json;

    namespace
    {
        std::string to_lower(std::string value)
        {
            std::transform(value.begin(), value.end(), value.begin(), [](unsigned char c) {
                return static_cast<char>(std::tolower(c));
            });
            return value;
        }

        float asFloat(const json &value, const std::string &field)
        {
            if (!value.is_number())
                throw LoadingSceneException(LoadingSceneException::ExceptionType::WRONG_FILE_CONTENT, field + " must be a number");
            return value.get<float>();
        }

        int asInt(const json &value, const std::string &field)
        {
            if (!value.is_number())
                throw LoadingSceneException(LoadingSceneException::ExceptionType::WRONG_FILE_CONTENT, field + " must be a number");
            return value.get<int>();
        }

        std::string asString(const json &value, const std::string &field)
        {
            if (!value.is_string())
                throw LoadingSceneException(LoadingSceneException::ExceptionType::WRONG_FILE_CONTENT, field + " must be a string");
            return value.get<std::string>();
        }

        Axis parseAxis(const json &value)
        {
            std::string axisStr = asString(value, "axis");

            if (axisStr == "x")
                return Axis::X;
            if (axisStr == "y")
                return Axis::Y;
            if (axisStr == "z")
                return Axis::Z;
            throw LoadingSceneException(LoadingSceneException::ExceptionType::WRONG_FILE_CONTENT, "Unknown axis \"" + axisStr + "\"");
        }

        // Link a freshly built leaf under a parent. The builder set its transform
        // as world; for a nested child that value is its LOCAL transform, so copy
        // it into the local fields (flatten recomputes world from the ancestors).
        void linkChild(ISceneObject *child, ISceneObject *parent)
        {
            if (!parent || !child)
                return;
            parent->addChild(child);
            child->setLocalPosition(child->getPosition());
            child->setLocalRotation(child->getRotation());
            child->setLocalScale(child->getScale());
        }
    }

    SceneParser::SceneParser()
    {
    }

    SceneParser::~SceneParser()
    {

    }

    json SceneParser::openConfig(const std::string &file_path)
    {
        std::ifstream file(file_path);

        if (!file)
            throw LoadingSceneException(LoadingSceneException::ExceptionType::CANNOT_OPEN_FILE);
        if (std::filesystem::path(file_path).extension() != ".json")
            throw LoadingSceneException(LoadingSceneException::ExceptionType::INVALID_FILE_EXTENSION);
        try
        {
            return json::parse(file);
        }
        catch (const json::parse_error &e)
        {
            throw LoadingSceneException(LoadingSceneException::ExceptionType::WRONG_FILE_CONTENT, e.what());
        }
    }

    Vector3f SceneParser::parseVector3f(const json &value)
    {
        Vector3f result;

        if (!value.is_array() || value.size() != 3)
            throw LoadingSceneException(LoadingSceneException::ExceptionType::WRONG_FILE_CONTENT, "expected an array of 3 numbers [x, y, z]");
        for (const auto &component : value)
            if (!component.is_number())
                throw LoadingSceneException(LoadingSceneException::ExceptionType::WRONG_FILE_CONTENT, "vector components must be numbers");
        result.x = value[0].get<float>();
        result.y = value[1].get<float>();
        result.z = value[2].get<float>();
        return (result);
    }

    Vector3i SceneParser::parseVector3i(const json &value)
    {
        Vector3i result;

        if (!value.is_array() || value.size() != 3)
            throw LoadingSceneException(LoadingSceneException::ExceptionType::WRONG_FILE_CONTENT, "expected an array of 3 numbers [x, y, z]");
        for (const auto &component : value)
            if (!component.is_number())
                throw LoadingSceneException(LoadingSceneException::ExceptionType::WRONG_FILE_CONTENT, "vector components must be numbers");
        result.x = value[0].get<int>();
        result.y = value[1].get<int>();
        result.z = value[2].get<int>();
        return (result);
    }

    Color SceneParser::parseColor(const json &value)
    {
        Color result;

        if (!value.is_array() || value.size() != 3)
            throw LoadingSceneException(LoadingSceneException::ExceptionType::WRONG_FILE_CONTENT, "expected an array of 3 numbers [r, g, b]");
        for (const auto &component : value)
            if (!component.is_number())
                throw LoadingSceneException(LoadingSceneException::ExceptionType::WRONG_FILE_CONTENT, "color components must be numbers");
        result.r = static_cast<uint8_t>(value[0].get<int>());
        result.g = static_cast<uint8_t>(value[1].get<int>());
        result.b = static_cast<uint8_t>(value[2].get<int>());
        result.a = 255;
        return (result);
    }

    Material SceneParser::parseMaterial(const json &object)
    {
        Material material;
        auto assignFloat = [&object](const char *key, float &field) {
            if (object.contains(key))
                field = asFloat(object[key], key);
        };

        if (object.contains("model"))
        {
            std::string lowered = to_lower(asString(object["model"], "model"));
            if (lowered == "phong")
                material.model = MaterialModel::PHONG;
            else if (lowered == "pbr")
                material.model = MaterialModel::PBR;
            else
                throw LoadingSceneException(LoadingSceneException::ExceptionType::WRONG_FILE_CONTENT, "Unknown material model: " + lowered);
        }
        if (object.contains("base_color"))
            material.baseColor = ColorF::fromColor(parseColor(object["base_color"]));
        if (object.contains("specular"))
            material.specular = ColorF::fromColor(parseColor(object["specular"]));

        assignFloat("shininess", material.shininess);
        assignFloat("reflectivity", material.reflectivity);
        assignFloat("transparency", material.transparency);
        assignFloat("ior", material.ior);
        assignFloat("metallic", material.metallic);
        assignFloat("roughness", material.roughness);
        assignFloat("ao", material.ao);
        assignFloat("specularLevel", material.specular_level);
        assignFloat("specularTint", material.specular_tint);
        assignFloat("clearcoat", material.clearcoat);
        assignFloat("clearcoatRoughness", material.clearcoat_roughness);
        assignFloat("sheen", material.sheen);
        assignFloat("sheenTint", material.sheen_tint);
        assignFloat("transmission", material.transmission);
        assignFloat("alpha", material.alpha);
        assignFloat("normal_scale", material.normal_scale);
        assignFloat("normal_noise_frequency", material.normal_noise_frequency);

        if (object.contains("normal_map"))
            material.normal_map = asString(object["normal_map"], "normal_map");
        if (object.contains("normal_map_enabled"))
        {
            const json &enabled = object["normal_map_enabled"];
            if (enabled.is_boolean())
                material.normal_map_enabled = enabled.get<bool>();
            else if (enabled.is_number())
                material.normal_map_enabled = (enabled.get<float>() != 0.0f);
        }

        return material;
    }

    Camera *SceneParser::parseCamera(const json &object)
    {
        Vector2i resolution;
        Vector3f position;
        Vector3f rotation;
        float fov;
        int samplesPerPixel = 5;

        if (!object.contains("resolution"))
            throw LoadingSceneException(LoadingSceneException::ExceptionType::WRONG_FILE_CONTENT, "Missing camera resolution");
        const json &resolutionSection = object["resolution"];
        if (!resolutionSection.is_array() || resolutionSection.size() != 2
            || !resolutionSection[0].is_number() || !resolutionSection[1].is_number())
            throw LoadingSceneException(LoadingSceneException::ExceptionType::WRONG_FILE_CONTENT, "Camera resolution must be [width, height]");
        resolution.x = resolutionSection[0].get<int>();
        resolution.y = resolutionSection[1].get<int>();

        if (!object.contains("position"))
            throw LoadingSceneException(LoadingSceneException::ExceptionType::WRONG_FILE_CONTENT, "Missing camera position");
        try
        {
            position = parseVector3f(object["position"]);
        }
        catch (std::exception &e)
        {
            throw LoadingSceneException(LoadingSceneException::ExceptionType::WRONG_FILE_CONTENT, "camera position: " + std::string(e.what()));
        }

        if (!object.contains("rotation"))
            throw LoadingSceneException(LoadingSceneException::ExceptionType::WRONG_FILE_CONTENT, "Missing camera rotation");
        try
        {
            rotation = parseVector3f(object["rotation"]);
        }
        catch (std::exception &e)
        {
            throw LoadingSceneException(LoadingSceneException::ExceptionType::WRONG_FILE_CONTENT, "camera rotation: " + std::string(e.what()));
        }

        if (!object.contains("fieldOfView"))
            throw LoadingSceneException(LoadingSceneException::ExceptionType::WRONG_FILE_CONTENT, "Missing camera fov");
        fov = asFloat(object["fieldOfView"], "fieldOfView");

        if (object.contains("samplesPerPixel"))
            samplesPerPixel = asInt(object["samplesPerPixel"], "samplesPerPixel");
        else if (object.contains("samples_per_pixel"))
            samplesPerPixel = asInt(object["samples_per_pixel"], "samples_per_pixel");

        if (samplesPerPixel <= 0)
            throw LoadingSceneException(LoadingSceneException::ExceptionType::WRONG_FILE_CONTENT, "camera samplesPerPixel must be greater than 0");

        return (new Camera(resolution, position, rotation, fov, samplesPerPixel));
    }

    std::vector<Material *> SceneParser::parseMaterials(const json &array)
    {
        std::vector<Material *> result;

        if (!array.is_array())
            throw LoadingSceneException(LoadingSceneException::ExceptionType::WRONG_FILE_CONTENT, "materials must be a list");

        for (const auto &materialSection : array)
        {
            if (!materialSection.contains("name"))
                throw LoadingSceneException(LoadingSceneException::ExceptionType::WRONG_FILE_CONTENT, "Missing material name");
            std::string name = asString(materialSection["name"], "material name");

            try
            {
                Material material = parseMaterial(materialSection);
                material.name = name;

                Material *matPtr = new Material(material);

                // Loaded materials are mirrored into the market so they can be
                // reused from other scenes (see MaterialLibrary).
                MaterialLibrary::save(*matPtr);

                this->_materials.push_back(matPtr);
                result.push_back(matPtr);
            }
            catch (std::exception &e)
            {
                std::cerr << LoadingSceneException(LoadingSceneException::ExceptionType::WRONG_FILE_CONTENT, "material \"" + name + "\": " + std::string(e.what())).what() << std::endl;
                continue;
            }
        }
        return result;
    }

    std::vector<ISceneObject *> SceneParser::parseObjects(const json &array)
    {
        std::vector<ISceneObject *> result;

        if (!array.is_array())
            throw LoadingSceneException(LoadingSceneException::ExceptionType::WRONG_FILE_CONTENT, "objects must be a list");

        for (const auto &object : array)
            this->parseObjectInto(object, result, nullptr);
        return result;
    }

    void SceneParser::parseObjectInto(const json &object, std::vector<ISceneObject *> &result, ISceneObject *parent)
    {
        using FieldHandler = std::function<void(SceneObjectBuilder &, const json &)>;
        static const std::vector<std::pair<std::string, FieldHandler>> handlers = {
            {"position",   [](SceneObjectBuilder &b, const json &v) { b.withPosition(parseVector3f(v)); }},
            {"rotation",   [](SceneObjectBuilder &b, const json &v) { b.withRotation(parseVector3f(v)); }},
            {"scale",      [](SceneObjectBuilder &b, const json &v) { b.withScale(parseVector3f(v)); }},
            {"direction",  [](SceneObjectBuilder &b, const json &v) { b.withDirection(parseVector3f(v)); }},
            {"vertex0",    [](SceneObjectBuilder &b, const json &v) { b.withVertex0(parseVector3f(v)); }},
            {"vertex1",    [](SceneObjectBuilder &b, const json &v) { b.withVertex1(parseVector3f(v)); }},
            {"vertex2",    [](SceneObjectBuilder &b, const json &v) { b.withVertex2(parseVector3f(v)); }},
            {"color",      [](SceneObjectBuilder &b, const json &v) { b.withColor(parseColor(v)); }},
            {"axis",       [](SceneObjectBuilder &b, const json &v) { b.withAxis(parseAxis(v)); }},
            {"radius",     [](SceneObjectBuilder &b, const json &v) { b.withRadius(asFloat(v, "radius")); }},
            {"height",     [](SceneObjectBuilder &b, const json &v) { b.withHeight(asFloat(v, "height")); }},
            {"size",       [](SceneObjectBuilder &b, const json &v) { b.withSize(asFloat(v, "size")); }},
            {"file",       [](SceneObjectBuilder &b, const json &v) { b.withFile(asString(v, "file")); }},
            {"vertex_overrides", [](SceneObjectBuilder &b, const json &v) {
                if (!v.is_array())
                    throw LoadingSceneException(LoadingSceneException::ExceptionType::WRONG_FILE_CONTENT, "vertex_overrides must be a list");
                std::vector<std::pair<int, Vector3f>> overrides;
                for (const auto &entry : v)
                {
                    if (!entry.contains("index") || !entry.contains("position"))
                        throw LoadingSceneException(LoadingSceneException::ExceptionType::WRONG_FILE_CONTENT, "vertex override needs an index and a position");
                    overrides.emplace_back(asInt(entry["index"], "index"), parseVector3f(entry["position"]));
                }
                b.withVertexOverrides(overrides);
            }},
            {"vertices", [](SceneObjectBuilder &b, const json &v) {
                if (!v.is_array())
                    throw LoadingSceneException(LoadingSceneException::ExceptionType::WRONG_FILE_CONTENT, "vertices must be a list");
                std::vector<Vector3f> vertices;
                for (const auto &entry : v)
                    vertices.push_back(parseVector3f(entry));
                b.withVertices(vertices);
            }},
            {"faces", [](SceneObjectBuilder &b, const json &v) {
                if (!v.is_array())
                    throw LoadingSceneException(LoadingSceneException::ExceptionType::WRONG_FILE_CONTENT, "faces must be a list");
                std::vector<std::array<int, 3>> faces;
                for (const auto &entry : v)
                {
                    if (!entry.is_array() || entry.size() != 3)
                        throw LoadingSceneException(LoadingSceneException::ExceptionType::WRONG_FILE_CONTENT, "a face must be an array of 3 vertex indices");
                    faces.push_back({asInt(entry[0], "face index"), asInt(entry[1], "face index"), asInt(entry[2], "face index")});
                }
                b.withFaces(faces);
            }},
            {"normals", [](SceneObjectBuilder &b, const json &v) {
                if (!v.is_array())
                    throw LoadingSceneException(LoadingSceneException::ExceptionType::WRONG_FILE_CONTENT, "normals must be a list");
                std::vector<Vector3f> normals;
                for (const auto &entry : v)
                    normals.push_back(parseVector3f(entry));
                b.withNormals(normals);
            }},
            {"power",      [](SceneObjectBuilder &b, const json &v) { b.withPower(asFloat(v, "power")); }},
            {"threshold",  [](SceneObjectBuilder &b, const json &v) { b.withThreshold(asFloat(v, "threshold")); }},
            {"intensity",  [](SceneObjectBuilder &b, const json &v) { b.withIntensity(asFloat(v, "intensity")); }},
            {"iterations", [](SceneObjectBuilder &b, const json &v) { b.withIterations(asInt(v, "iterations")); }},
        };

        if (!object.contains("type") || !object["type"].is_string())
        {
            std::cerr << LoadingSceneException(LoadingSceneException::ExceptionType::WRONG_FILE_CONTENT, "Missing object type").what() << std::endl;
            return;
        }
        std::string type = object["type"].get<std::string>();

        // Group node: owns a local transform and builds its children recursively.
        if (type == "group")
        {
            Group *group = new Group();

            if (object.contains("name"))
            {
                try { group->setName(asString(object["name"], "name")); }
                catch (std::exception &e) { std::cerr << LoadingSceneException(LoadingSceneException::ExceptionType::WRONG_FILE_CONTENT, std::string("group name: ") + e.what()).what() << std::endl; }
            }
            // Parent must be set before the local setters so they store LOCAL
            // (for a root group parent is null and they store world instead).
            if (parent)
                parent->addChild(group);
            try
            {
                if (object.contains("position"))
                    group->setLocalPosition(parseVector3f(object["position"]));
                if (object.contains("rotation"))
                    group->setLocalRotation(parseVector3f(object["rotation"]));
                if (object.contains("scale"))
                    group->setLocalScale(parseVector3f(object["scale"]));
            }
            catch (std::exception &e)
            {
                std::cerr << LoadingSceneException(LoadingSceneException::ExceptionType::WRONG_FILE_CONTENT, std::string("group transform: ") + e.what()).what() << std::endl;
            }
            result.push_back(group);

            if (object.contains("children") && object["children"].is_array())
                for (const auto &child : object["children"])
                    this->parseObjectInto(child, result, group);
            return;
        }

        SceneObjectBuilder builder;

        try
        {
            if (type == LIGHT_POINT)
                builder.withType(LightType::POINT);
            else if (type == LIGHT_DIRECTIONAL)
                builder.withType(LightType::DIRECTIONAL);
            else if (type == "obj")
            {
                if (!object.contains("path"))
                    throw LoadingSceneException(LoadingSceneException::ExceptionType::WRONG_FILE_CONTENT, "Missing object path");

                std::string path = asString(object["path"], "path");
                std::vector<ISceneObject *> objResult;
                ObjParser objParser(objResult);

                if (object.contains("position"))
                    objParser.withPosition(parseVector3f(object["position"]));
                if (object.contains("rotation"))
                    objParser.withRotation(parseVector3f(object["rotation"]));
                if (object.contains("size"))
                    objParser.withSize(asFloat(object["size"], "size"));

                objParser.parse(path);
                for (ISceneObject *triangle : objResult)
                {
                    linkChild(triangle, parent);
                    result.push_back(triangle);
                }
                return;
            }
            else
                builder.withType(type);
        }
        catch (std::exception &e)
        {
            std::cerr << LoadingSceneException(LoadingSceneException::ExceptionType::WRONG_FILE_CONTENT, "object \"" + type + "\": " + std::string(e.what())).what() << std::endl;
            return;
        }

        if (object.contains("name"))
        {
            try
            {
                builder.withName(asString(object["name"], "name"));
            }
            catch (std::exception &e)
            {
                std::cerr << LoadingSceneException(LoadingSceneException::ExceptionType::WRONG_FILE_CONTENT, "object \"" + type + "\" name: " + std::string(e.what())).what() << std::endl;
                return;
            }
        }

        for (const auto &handler : handlers)
        {
            const std::string &key = handler.first;

            if (!object.contains(key))
                continue;
            try
            {
                handler.second(builder, object[key]);
            }
            catch (std::exception &e)
            {
                std::cerr << LoadingSceneException(LoadingSceneException::ExceptionType::WRONG_FILE_CONTENT, "object \"" + type + "\" " + key + ": " + std::string(e.what())).what() << std::endl;
                return;
            }
        }

        if (object.contains("material"))
        {
            try
            {
                std::string materialName = asString(object["material"], "material");
                auto it = std::find_if(this->_materials.begin(), this->_materials.end(), [&materialName](const Material *material)
                {
                    return material != nullptr && material->getName() == materialName;
                });
                if (it == this->_materials.end())
                    throw LoadingSceneException(LoadingSceneException::ExceptionType::WRONG_FILE_CONTENT, "Unknown material \"" + materialName + "\"");
                builder.withMaterial(*it);
            }
            catch (std::exception &e)
            {
                std::cerr << LoadingSceneException(LoadingSceneException::ExceptionType::WRONG_FILE_CONTENT, "object \"" + type + "\" material: " + std::string(e.what())).what() << std::endl;
                return;
            }
        }

        try
        {
            ISceneObject *sceneObject = builder.build();
            if (sceneObject == nullptr)
                throw LoadingSceneException(LoadingSceneException::ExceptionType::WRONG_FILE_CONTENT, "object \"" + type + "\": Unknown or incomplete object definition");
            linkChild(sceneObject, parent);
            result.push_back(sceneObject);
        }
        catch (std::exception &e)
        {
            std::cerr << LoadingSceneException(LoadingSceneException::ExceptionType::WRONG_FILE_CONTENT, "object \"" + type + "\": " + std::string(e.what())).what() << std::endl;
            return;
        }
    }

    IScene *SceneParser::parseScene(const std::string &scene_path)
    {
        return (this->parseScene(this->openConfig(scene_path)));
    }

    IScene *SceneParser::parseScene(const json &config)
    {
        SceneBuilder scene_builder;
        float ambientCoefficient = 0.4f;
        float diffuseCoefficient = 0.6f;

        if (!config.contains("camera"))
            throw LoadingSceneException(LoadingSceneException::ExceptionType::WRONG_FILE_CONTENT, "Missing camera section");
        if (!config.contains("objects"))
            throw LoadingSceneException(LoadingSceneException::ExceptionType::WRONG_FILE_CONTENT, "Missing objects section");

        if (config.contains("environment"))
        {
            const json &environmentSection = config["environment"];
            if (environmentSection.contains("ambient") && environmentSection["ambient"].is_number())
                ambientCoefficient = environmentSection["ambient"].get<float>();
            if (environmentSection.contains("diffuse") && environmentSection["diffuse"].is_number())
                diffuseCoefficient = environmentSection["diffuse"].get<float>();
        }

        scene_builder
            .withCamera(parseCamera(config["camera"]))
            .withMaterials(config.contains("materials") ? parseMaterials(config["materials"]) : std::vector<Material *>{})
            .withObjects(parseObjects(config["objects"]))
            .withAmbientCoefficient(ambientCoefficient)
            .withDiffuseCoefficient(diffuseCoefficient);
        return (scene_builder.build());
    }

}
