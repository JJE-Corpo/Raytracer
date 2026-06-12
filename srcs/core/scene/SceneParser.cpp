//
// Created by jazema on 4/24/26.
//

#include "SceneParser.hpp"

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <libconfig.h++>

#include "Scene.hpp"
#include "../../common/Color.hpp"
#include "builder/SceneBuilder.hpp"
#include "../exceptions/LoadingSceneException.hpp"
#include "builder/LightBuilder.hpp"
#include "builder/PrimitiveBuilder.hpp"
#include "builder/SceneObjectBuilder.hpp"
#include "../../core/PluginLoader.hpp"
#include "../../common/Material.hpp"
#include "../obj/ObjParser.hpp"
#include "../../common/scene/ISceneObject.hpp"

namespace rc
{
    namespace
    {
        std::string to_lower(std::string value)
        {
            std::transform(value.begin(), value.end(), value.begin(), [](unsigned char c) {
                return static_cast<char>(std::tolower(c));
            });
            return value;
        }
    }

    SceneParser::SceneParser()
    {
    }

    SceneParser::~SceneParser()
    {

    }

    void SceneParser::openConfig(libconfig::Config &config, const std::string &file_path)
    {
        std::ifstream file(file_path);

        if (!file)
            throw LoadingSceneException(LoadingSceneException::ExceptionType::CANNOT_OPEN_FILE);
        file.close();
        if (std::filesystem::path(file_path).extension() != ".cfg")
            throw LoadingSceneException(LoadingSceneException::ExceptionType::INVALID_FILE_EXTENSION);
        try
        {
            config.readFile(file_path.c_str());
        }
        catch (const libconfig::FileIOException &e)
        {
            throw LoadingSceneException(LoadingSceneException::ExceptionType::CANNOT_OPEN_FILE);
        }
        catch (const libconfig::ParseException &e)
        {
            throw LoadingSceneException(LoadingSceneException::ExceptionType::WRONG_FILE_CONTENT, "[Line " + std::to_string(e.getLine()) + "] " + e.getError());
        }
    }

    Vector3f SceneParser::parseVector3f(const libconfig::Setting &section)
    {
        Vector3f result;

        if (!section.lookupValue("x", result.x))
            throw LoadingSceneException(LoadingSceneException::ExceptionType::WRONG_FILE_CONTENT, "Missing position x");
        if (!section.lookupValue("y", result.y))
            throw LoadingSceneException(LoadingSceneException::ExceptionType::WRONG_FILE_CONTENT, "Missing position y");
        if (!section.lookupValue("z", result.z))
            throw LoadingSceneException(LoadingSceneException::ExceptionType::WRONG_FILE_CONTENT, "Missing position z");
        if (section.getLength() != 3)
            throw LoadingSceneException(LoadingSceneException::ExceptionType::WRONG_FILE_CONTENT, "Vector should have exactly 3 values (x,y,z)");
        return (result);
    }

    Vector3i SceneParser::parseVector3i(const libconfig::Setting &section)
    {
        Vector3i result;

        if (!section.lookupValue("x", result.x))
            throw LoadingSceneException(LoadingSceneException::ExceptionType::WRONG_FILE_CONTENT, "Missing position x");
        if (!section.lookupValue("y", result.y))
            throw LoadingSceneException(LoadingSceneException::ExceptionType::WRONG_FILE_CONTENT, "Missing position y");
        if (!section.lookupValue("z", result.z))
            throw LoadingSceneException(LoadingSceneException::ExceptionType::WRONG_FILE_CONTENT, "Missing position z");
        if (section.getLength() != 3)
            throw LoadingSceneException(LoadingSceneException::ExceptionType::WRONG_FILE_CONTENT, "Vector should have exactly 3 values (x,y,z)");
        return (result);
    }

    Color SceneParser::parseColor(const libconfig::Setting &section)
    {
        Color result;
        int r, g, b;

        if (!section.lookupValue("r", r))
            throw LoadingSceneException(LoadingSceneException::ExceptionType::WRONG_FILE_CONTENT, "Missing colr r");
        if (!section.lookupValue("g", g))
            throw LoadingSceneException(LoadingSceneException::ExceptionType::WRONG_FILE_CONTENT, "Missing color g");
        if (!section.lookupValue("b", b))
            throw LoadingSceneException(LoadingSceneException::ExceptionType::WRONG_FILE_CONTENT, "Missing color b");
        result.r = static_cast<uint8_t>(r);
        result.g = static_cast<uint8_t>(g);
        result.b = static_cast<uint8_t>(b);
        result.a = 255;
        if (section.getLength() != 3)
            throw LoadingSceneException(LoadingSceneException::ExceptionType::WRONG_FILE_CONTENT, "Vector should have exactly 3 values (x,y,z)");
        return (result);
    }

    Material SceneParser::parseMaterial(const libconfig::Setting &section)
    {
        Material material;

        if (section.exists("model"))
        {
            std::string model;
            if (section.lookupValue("model", model))
            {
                std::string lowered = to_lower(model);
                if (lowered == "phong")
                    material.model = MaterialModel::PHONG;
                else if (lowered == "pbr")
                    material.model = MaterialModel::PBR;
                else
                    throw LoadingSceneException(LoadingSceneException::ExceptionType::WRONG_FILE_CONTENT, "Unknown material model: " + model);
            }
        }
        if (section.exists("base_color"))
            material.baseColor = ColorF::fromColor(parseColor(section.lookup("base_color")));
        if (section.exists("specular"))
            material.specular = ColorF::fromColor(parseColor(section.lookup("specular")));
        if (section.exists("shininess"))
            section.lookupValue("shininess", material.shininess);
        if (section.exists("reflectivity"))
            section.lookupValue("reflectivity", material.reflectivity);
        if (section.exists("transparency"))
            section.lookupValue("transparency", material.transparency);
        if (section.exists("ior"))
            section.lookupValue("ior", material.ior);
        if (section.exists("metallic"))
            section.lookupValue("metallic", material.metallic);
        if (section.exists("roughness"))
            section.lookupValue("roughness", material.roughness);
        if (section.exists("ao"))
            section.lookupValue("ao", material.ao);
        if (section.exists("specularLevel"))
            section.lookupValue("specularLevel", material.specular_level);
        if (section.exists("specularTint"))
            section.lookupValue("specularTint", material.specular_tint);
        if (section.exists("clearcoat"))
            section.lookupValue("clearcoat", material.clearcoat);
        if (section.exists("clearcoatRoughness"))
            section.lookupValue("clearcoatRoughness", material.clearcoat_roughness);
        if (section.exists("sheen"))
            section.lookupValue("sheen", material.sheen);
        if (section.exists("sheenTint"))
            section.lookupValue("sheenTint", material.sheen_tint);
        if (section.exists("transmission"))
            section.lookupValue("transmission", material.transmission);
        if (section.exists("alpha"))
            section.lookupValue("alpha", material.alpha);

        if (section.exists("normal_map"))
            material.normal_map = section.lookup("normal_map").c_str();
        if (section.exists("normal_map_enabled"))
        {
            try { float v = 0.0f; section.lookupValue("normal_map_enabled", v); material.normal_map_enabled = (v != 0.0f); }
            catch (...) { bool b = false; section.lookupValue("normal_map_enabled", b); material.normal_map_enabled = b; }
        }
        if (section.exists("normal_scale"))
            section.lookupValue("normal_scale", material.normal_scale);
        if (section.exists("normal_noise_frequency"))
            section.lookupValue("normal_noise_frequency", material.normal_noise_frequency);

        return material;
    }

    Camera *SceneParser::parseCamera(const libconfig::Setting &section)
    {
        Vector2i resolution;
        Vector3f position;
        Vector3f rotation;
        float fov;
        int samplesPerPixel = 5;

        const libconfig::Setting &resolutionSection = section.lookup("resolution");
        const libconfig::Setting &positionSection = section.lookup("position");
        const libconfig::Setting &rotationSection = section.lookup("rotation");

        if (!resolutionSection.lookupValue("width", resolution.x))
            throw LoadingSceneException(LoadingSceneException::ExceptionType::WRONG_FILE_CONTENT, "Missing camera resolution width");
        if (!resolutionSection.lookupValue("height", resolution.y))
            throw LoadingSceneException(LoadingSceneException::ExceptionType::WRONG_FILE_CONTENT, "Missing camera resolution height");
        if (resolutionSection.getLength() != 2)
            throw LoadingSceneException(LoadingSceneException::ExceptionType::WRONG_FILE_CONTENT, "Camera resolution must have exactly 2 values");

        try
        {
            position = parseVector3f(positionSection);
        }
        catch (std::exception &e)
        {
            throw LoadingSceneException(LoadingSceneException::ExceptionType::WRONG_FILE_CONTENT, "camera position: " + std::string(e.what()));
        }

        try
        {
            rotation = parseVector3f(rotationSection);
        }
        catch (std::exception &e)
        {
            throw LoadingSceneException(LoadingSceneException::ExceptionType::WRONG_FILE_CONTENT, "camera rotation: " + std::string(e.what()));
        }

        if (!section.lookupValue("fieldOfView", fov))
            throw LoadingSceneException(LoadingSceneException::ExceptionType::WRONG_FILE_CONTENT, "Missing camera fov");

        if (section.exists("samplesPerPixel"))
        {
            if (!section.lookupValue("samplesPerPixel", samplesPerPixel))
                throw LoadingSceneException(LoadingSceneException::ExceptionType::WRONG_FILE_CONTENT, "Missing camera samplesPerPixel");
        }
        else if (section.exists("samples_per_pixel"))
        {
            if (!section.lookupValue("samples_per_pixel", samplesPerPixel))
                throw LoadingSceneException(LoadingSceneException::ExceptionType::WRONG_FILE_CONTENT, "Missing camera samples_per_pixel");
        }

        if (samplesPerPixel <= 0)
            throw LoadingSceneException(LoadingSceneException::ExceptionType::WRONG_FILE_CONTENT, "camera samplesPerPixel must be greater than 0");

        return (new Camera(resolution, position, rotation, fov, samplesPerPixel));
    }

    std::vector<Material *> SceneParser::parseMaterials(const libconfig::Setting &section)
    {
        std::vector<Material *> result;

        for (int i = 0; i < section.getLength(); i++)
        {
            const libconfig::Setting &materialSection = section[i];

            if (!materialSection.exists("name"))
                throw LoadingSceneException(LoadingSceneException::ExceptionType::WRONG_FILE_CONTENT, "Missing material name");
            std::string name = materialSection.lookup("name").c_str();

            try
            {
                Material material = parseMaterial(materialSection);
                material.name = name;

                Material *matPtr = new Material(material);

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

    std::vector <ISceneObject *> SceneParser::parseObjects(const libconfig::Setting &section)
    {
        std::vector<ISceneObject *> result;

        for (int i = 0; i < section.getLength(); i++)
        {
            SceneObjectBuilder builder;
            const libconfig::Setting &objectSection = section[i];

            if (!objectSection.exists("type"))
            {
                std::cerr << LoadingSceneException(LoadingSceneException::ExceptionType::WRONG_FILE_CONTENT, "Missing object type").what() << std::endl;
                continue;
            }
            std::string type = objectSection.lookup("type").c_str();

            try
            {
                if (type == LIGHT_POINT)
                    builder.withType(LightType::POINT);
                else if (type == LIGHT_DIRECTIONAL)
                    builder.withType(LightType::DIRECTIONAL);
                else if (type == "obj")
                {
                    if (!objectSection.exists("path"))
                        throw LoadingSceneException(LoadingSceneException::ExceptionType::WRONG_FILE_CONTENT, "Missing object path");
                    
                    std::string path = objectSection.lookup("path").c_str();
                    ObjParser objParser(result);

                    if (objectSection.exists("position"))
                    {
                        try
                        {
                            objParser.withPosition(parseVector3f(objectSection.lookup("position")));
                        }
                        catch (std::exception &e)
                        {
                            std::cerr << LoadingSceneException(LoadingSceneException::ExceptionType::WRONG_FILE_CONTENT, "object \"" + type + "\" position: " + std::string(e.what())).what() << std::endl;
                            continue;
                        }
                    }

                    if (objectSection.exists("rotation"))
                    {
                        try
                        {
                            objParser.withRotation(parseVector3f(objectSection.lookup("rotation")));
                        }
                        catch (std::exception &e)
                        {
                            std::cerr << LoadingSceneException(LoadingSceneException::ExceptionType::WRONG_FILE_CONTENT, "object \"" + type + "\" rotation: " + std::string(e.what())).what() << std::endl;
                            continue;
                        }
                    }

                    if (objectSection.exists("size"))
                    {
                        try
                        {
                            objParser.withSize(objectSection.lookup("size"));
                        }
                        catch (std::exception &e)
                        {
                            std::cerr << LoadingSceneException(LoadingSceneException::ExceptionType::WRONG_FILE_CONTENT, "object \"" + type + "\" size: " + std::string(e.what())).what() << std::endl;
                            continue;
                        }
                    }

                    objParser.parse(path);

                    continue;
                }
                else
                    builder.withType(type);
            }
            catch (std::exception &e)
            {
                std::cerr << LoadingSceneException(LoadingSceneException::ExceptionType::WRONG_FILE_CONTENT, "object \"" + type + "\": " + std::string(e.what())).what() << std::endl;
                continue;
            }

            if (objectSection.exists("name"))
            {
                try
                {
                    builder.withName(objectSection.lookup("name").c_str());
                }
                catch (std::exception &e)
                {
                    std::cerr << LoadingSceneException(LoadingSceneException::ExceptionType::WRONG_FILE_CONTENT, "object \"" + type + "\" name: " + std::string(e.what())).what() << std::endl;
                    continue;
                }
            }

            if (objectSection.exists("position"))
            {
                try
                {
                    builder.withPosition(parseVector3f(objectSection.lookup("position")));
                }
                catch (std::exception &e)
                {
                    std::cerr << LoadingSceneException(LoadingSceneException::ExceptionType::WRONG_FILE_CONTENT, "object \"" + type + "\" position: " + std::string(e.what())).what() << std::endl;
                    continue;
                }
            }

            if (objectSection.exists("rotation"))
            {
                try
                {
                    builder.withRotation(parseVector3f(objectSection.lookup("rotation")));
                }
                catch (std::exception &e)
                {
                    std::cerr << LoadingSceneException(LoadingSceneException::ExceptionType::WRONG_FILE_CONTENT, "object \"" + type + "\" rotation: " + std::string(e.what())).what() << std::endl;
                    continue;
                }
            }

            if (objectSection.exists("scale"))
            {
                try
                {
                    builder.withScale(parseVector3f(objectSection.lookup("scale")));
                }
                catch (std::exception &e)
                {
                    std::cerr << LoadingSceneException(LoadingSceneException::ExceptionType::WRONG_FILE_CONTENT, "object \"" + type + "\" scale: " + std::string(e.what())).what() << std::endl;
                    continue;
                }
            }

            if (objectSection.exists("material"))
            {
                try
                {
                    std::string materialName = objectSection.lookup("material").c_str();
                    auto it = std::find_if(this->_materials.begin(), this->_materials.end(), [&materialName](const Material *material)
                    {
                        return material != nullptr && material->getName() == materialName;
                    });
                    if (it == this->_materials.end())
                        throw LoadingSceneException(LoadingSceneException::ExceptionType::WRONG_FILE_CONTENT, "object \"" + type + "\" material: Unknown material \"" + materialName + "\"");
                    builder.withMaterial(*it);
                }
                catch (std::exception &e)
                {
                    std::cerr << LoadingSceneException(LoadingSceneException::ExceptionType::WRONG_FILE_CONTENT, "object \"" + type + "\" material: " + std::string(e.what())).what() << std::endl;
                    continue;
                }
            }

            if (objectSection.exists("power"))
            {
                try
                {
                    builder.withPower(objectSection.lookup("power"));
                }
                catch (std::exception &e)
                {
                    std::cerr << LoadingSceneException(LoadingSceneException::ExceptionType::WRONG_FILE_CONTENT, "object \"" + type + "\" power: " + std::string(e.what())).what() << std::endl;
                    continue;
                }
            }

            if (objectSection.exists("iterations"))
            {
                try
                {
                    builder.withIterations(objectSection.lookup("iterations"));
                }
                catch (std::exception &e)
                {
                    std::cerr << LoadingSceneException(LoadingSceneException::ExceptionType::WRONG_FILE_CONTENT, "object \"" + type + "\" iterations: " + std::string(e.what())).what() << std::endl;
                    continue;
                }
            }

            if (objectSection.exists("threshold"))
            {
                try
                {
                    builder.withThreshold(objectSection.lookup("threshold"));
                }
                catch (std::exception &e)
                {
                    std::cerr << LoadingSceneException(LoadingSceneException::ExceptionType::WRONG_FILE_CONTENT, "object \"" + type + "\" threshold: " + std::string(e.what())).what() << std::endl;
                    continue;
                }
            }

            if (objectSection.exists("direction"))
            {
                try
                {
                    builder.withDirection(parseVector3f(objectSection.lookup("direction")));
                }
                catch (std::exception &e)
                {
                    std::cerr << LoadingSceneException(LoadingSceneException::ExceptionType::WRONG_FILE_CONTENT, "object \"" + type + "\" direction: " + std::string(e.what())).what() << std::endl;
                    continue;
                }
            }

            if (objectSection.exists("intensity"))
            {
                try
                {
                    builder.withIntensity(objectSection.lookup("intensity"));
                }
                catch (std::exception &e)
                {
                    std::cerr << LoadingSceneException(LoadingSceneException::ExceptionType::WRONG_FILE_CONTENT, "object \"" + type + "\" intensity: " + std::string(e.what())).what() << std::endl;
                    continue;
                }
            }

            if (objectSection.exists("size"))
            {
                try
                {
                    builder.withSize(objectSection.lookup("size"));
                }
                catch (std::exception &e)
                {
                    std::cerr << LoadingSceneException(LoadingSceneException::ExceptionType::WRONG_FILE_CONTENT, "object \"" + type + "\" size: " + std::string(e.what())).what() << std::endl;
                    continue;
                }
            }

            if (objectSection.exists("height"))
            {
                try
                {
                    builder.withHeight(objectSection.lookup("height"));
                }
                catch (std::exception &e)
                {
                    std::cerr << LoadingSceneException(LoadingSceneException::ExceptionType::WRONG_FILE_CONTENT, "object \"" + type + "\" height: " + std::string(e.what())).what() << std::endl;
                    continue;
                }
            }

            if (objectSection.exists("radius"))
            {
                try
                {
                    builder.withRadius(objectSection.lookup("radius"));
                }
                catch (std::exception &e)
                {
                    try
                    {
                        int radiusInt = objectSection.lookup("radius");
                        
                        builder.withRadius(static_cast<float>(radiusInt));
                    }
                    catch (std::exception &e)
                    {
                        std::cerr << LoadingSceneException(LoadingSceneException::ExceptionType::WRONG_FILE_CONTENT, "object \"" + type + "\" radius: " + std::string(e.what())).what() << std::endl;
                        continue;
                    }
                }
            }

            if (objectSection.exists("vertex0"))
            {
                try
                {
                    builder.withVertex0(parseVector3f(objectSection.lookup("vertex0")));
                }
                catch (std::exception &e)
                {
                    std::cerr << LoadingSceneException(LoadingSceneException::ExceptionType::WRONG_FILE_CONTENT, "object \"" + type + "\" vertex0: " + std::string(e.what())).what() << std::endl;
                    continue;
                }
            }

            if (objectSection.exists("vertex1"))
            {
                try
                {
                    builder.withVertex1(parseVector3f(objectSection.lookup("vertex1")));
                }
                catch (std::exception &e)
                {
                    std::cerr << LoadingSceneException(LoadingSceneException::ExceptionType::WRONG_FILE_CONTENT, "object \"" + type + "\" vertex1: " + std::string(e.what())).what() << std::endl;
                    continue;
                }
            }

            if (objectSection.exists("vertex2"))
            {
                try
                {
                    builder.withVertex2(parseVector3f(objectSection.lookup("vertex2")));
                }
                catch (std::exception &e)
                {
                    std::cerr << LoadingSceneException(LoadingSceneException::ExceptionType::WRONG_FILE_CONTENT, "object \"" + type + "\" vertex2: " + std::string(e.what())).what() << std::endl;
                    continue;
                }
            }

            if (objectSection.exists("axis"))
            {
                try
                {
                    std::string axisStr = objectSection.lookup("axis").c_str();
                    Axis axis;
                    if (axisStr == "x")
                        axis = Axis::X;
                    else if (axisStr == "y")
                        axis = Axis::Y;
                    else if (axisStr == "z")
                        axis = Axis::Z;
                    else
                        throw LoadingSceneException(LoadingSceneException::ExceptionType::WRONG_FILE_CONTENT, "object \"" + type + "\" axis: Unknown axis \"" + axisStr + "\"");
                    builder.withAxis(axis);
                }
                catch (std::exception &e)
                {
                    std::cerr << LoadingSceneException(LoadingSceneException::ExceptionType::WRONG_FILE_CONTENT, "object \"" + type + "\" axis: " + std::string(e.what())).what() << std::endl;
                    continue;
                }
            }

            if (objectSection.exists("color"))
            {
                try
                {
                    builder.withColor(parseColor(objectSection.lookup("color")));
                }
                catch (std::exception &e)
                {
                    std::cerr << LoadingSceneException(LoadingSceneException::ExceptionType::WRONG_FILE_CONTENT, "object \"" + type + "\" color: " + std::string(e.what())).what() << std::endl;
                    continue;
                }
            }

            try
            {
                ISceneObject *object = builder.build();
                if (object == nullptr)
                    throw LoadingSceneException(LoadingSceneException::ExceptionType::WRONG_FILE_CONTENT, "object \"" + type + "\": Unknown or incomplete object definition");
                result.push_back(object);
            }
            catch (std::exception &e)
            {
                std::cerr << LoadingSceneException(LoadingSceneException::ExceptionType::WRONG_FILE_CONTENT, "object \"" + type + "\": " + std::string(e.what())).what() << std::endl;
                continue;
            }
        }
        return result;
    }

    IScene *SceneParser::parseScene(const std::string &scene_path)
    {
        libconfig::Config config;

        this->openConfig(config, scene_path);
        return (this->parseScene(config));
    }

    IScene *SceneParser::parseScene(const libconfig::Config &config)
    {
        SceneBuilder scene_builder;
        float ambientCoefficient = 0.4f;
        float diffuseCoefficient = 0.6f;

        const libconfig::Setting &environmentSection = config.lookup("environment");
        environmentSection.lookupValue("ambient", ambientCoefficient);
        environmentSection.lookupValue("diffuse", diffuseCoefficient);
        scene_builder
            .withCamera(parseCamera(config.lookup("camera")))
            .withMaterials(parseMaterials(config.lookup("materials")))
            .withObjects(parseObjects(config.lookup("objects")))
            .withAmbientCoefficient(ambientCoefficient)
            .withDiffuseCoefficient(diffuseCoefficient);
        return (scene_builder.build());
    }

}
