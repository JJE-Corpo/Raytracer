#include "SceneRegister.hpp"

#include <filesystem>

#include "../../plugins/light/DirectionalLight.hpp"

#include <fstream>

rc::SceneRegister::SceneRegister()
{
}

rc::SceneRegister::~SceneRegister()
{
}

void rc::SceneRegister::registerVector3f(std::string name, Vector3f vector, libconfig::Setting &setting)
{
    libconfig::Setting &vectorSetting = setting.add(name, libconfig::Setting::TypeGroup);

    vectorSetting.add("x", libconfig::Setting::TypeFloat) = vector.x;
    vectorSetting.add("y", libconfig::Setting::TypeFloat) = vector.y;
    vectorSetting.add("z", libconfig::Setting::TypeFloat) = vector.z;
}

void rc::SceneRegister::registerVector3i(std::string name, Vector3i vector, libconfig::Setting &setting)
{
    libconfig::Setting &vectorSetting = setting.add(name, libconfig::Setting::TypeGroup);

    vectorSetting.add("x", libconfig::Setting::TypeInt) = vector.x;
    vectorSetting.add("y", libconfig::Setting::TypeInt) = vector.y;
    vectorSetting.add("z", libconfig::Setting::TypeInt) = vector.z;
}

void rc::SceneRegister::registerVector2i(std::string name, Vector2i vector, libconfig::Setting &setting)
{
    libconfig::Setting &vectorSetting = setting.add(name, libconfig::Setting::TypeGroup);

    vectorSetting.add("width", libconfig::Setting::TypeInt) = vector.x;
    vectorSetting.add("height", libconfig::Setting::TypeInt) = vector.y;
}

void rc::SceneRegister::registerVector2f(std::string name, Vector2f vector, libconfig::Setting &setting)
{
    libconfig::Setting &vectorSetting = setting.add(name, libconfig::Setting::TypeGroup);

    vectorSetting.add("x", libconfig::Setting::TypeFloat) = vector.x;
    vectorSetting.add("y", libconfig::Setting::TypeFloat) = vector.y;
}

void rc::SceneRegister::registerColor(std::string name, Color color, libconfig::Setting &setting)
{
    libconfig::Setting &colorSetting = setting.add(name, libconfig::Setting::TypeGroup);

    colorSetting.add("r", libconfig::Setting::TypeInt) = color.r;
    colorSetting.add("g", libconfig::Setting::TypeInt) = color.g;
    colorSetting.add("b", libconfig::Setting::TypeInt) = color.b;
}

void rc::SceneRegister::registerInt(std::string name, int value, libconfig::Setting &setting)
{
    setting.add(name, libconfig::Setting::TypeInt) = value;
}

void rc::SceneRegister::registerFloat(std::string name, float value, libconfig::Setting &setting)
{
    setting.add(name, libconfig::Setting::TypeFloat) = value;
}

void rc::SceneRegister::registerString(const std::string &name, const std::string &value, libconfig::Setting &setting)
{
    setting.add(name, libconfig::Setting::TypeString) = value;
}

libconfig::Setting &rc::SceneRegister::getRoot()
{
    return this->_cfg.getRoot();
}

void rc::SceneRegister::registerCamera(ICamera *camera)
{
    libconfig::Setting &root = this->getRoot();

    root.add("camera", libconfig::Setting::TypeGroup);

    this->registerVector2i("resolution", camera->getResolution(), root["camera"]);
    this->registerVector3f("position", camera->getPosition(), root["camera"]);
    this->registerVector3f("rotation", camera->getRotation(), root["camera"]);
    this->registerFloat("fieldOfView", camera->getFov(), root["camera"]);
    this->registerInt("samplesPerPixel", camera->getSamplesPerPixel(), root["camera"]);
}

void rc::SceneRegister::registerPrimitive(IPrimitive *primitive)
{
    libconfig::Setting &root = this->getRoot();

    libconfig::Setting &primitivesSection = root["objects"];
    
    libconfig::Setting &primitiveSection = primitivesSection.add(libconfig::Setting::TypeGroup);

    this->registerString("name", primitive->getName(), primitiveSection);
    this->registerString("type", primitive->getTypeName(), primitiveSection);

    const Material *material = primitive->getMaterial();

    if (material)
        this->registerString("material", material->getName(), primitiveSection);

    std::map<std::string, std::pair<std::string, PropertyType>> properties = primitive->getProperties();

    for (const auto &property : properties)
    {
        const std::string &name = property.first;
        const std::string &value = property.second.first;
        const PropertyType &type = property.second.second;
        libconfig::Setting *target = &primitiveSection;
        std::string targetName = name;

        switch (type)
        {
            case PropertyType::COLOR:
                this->registerColor(targetName, Color(value), *target);
                break;
            case PropertyType::VECTOR3F:
                this->registerVector3f(targetName, Vector3f(value), *target);
                break;
            case PropertyType::VECTOR3I:
                this->registerVector3i(targetName, Vector3i(value), *target);
                break;
            case PropertyType::VECTOR2F:
                this->registerVector2f(targetName, Vector2f(value), *target);
                break;
            case PropertyType::VECTOR2I:
                this->registerVector2i(targetName, Vector2i(value), *target);
                break;
            case PropertyType::FLOAT:
                this->registerFloat(targetName, std::stof(value), *target);
                break;
            case PropertyType::INT:
                this->registerInt(targetName, std::stoi(value), *target);
                break;
            case PropertyType::STRING:
                this->registerString(targetName, value, *target);
                break;
        }
    }
}

void rc::SceneRegister::registerLight(ILight *light)
{
    libconfig::Setting &root = this->getRoot();

    libconfig::Setting &lightsSection = root["objects"];

    libconfig::Setting &lightSection = lightsSection.add(libconfig::Setting::TypeGroup);

    switch (light->getKind())
    {
        case LightKind::DIRECTIONAL:
        {
            // DirectionalLight *directional_light = dynamic_cast<DirectionalLight*>(light);
            this->registerString("name", light->getName(), lightSection);
            this->registerString("type", light->getTypeName(), lightSection);
            this->registerVector3f("direction", light->getRotation(), lightSection);
            this->registerColor("color", light->getColorF().toColor(), lightSection);
            this->registerFloat("intensity", light->getIntensity(), lightSection);
            break;
        }
        case LightKind::POINT:
        {
            this->registerString("name", light->getName(), lightSection);
            this->registerString("type", light->getTypeName(), lightSection);
            this->registerVector3f("position", light->getPosition(), lightSection);
            this->registerColor("color", light->getColorF().toColor(), lightSection);
            this->registerFloat("intensity", light->getIntensity(), lightSection);
            break;
        }
        default:
            std::cerr << "Unknown light type, cannot register" << std::endl;
    }
}

void rc::SceneRegister::registerMaterial(const Material *material)
{
    libconfig::Setting &root = this->getRoot();

    libconfig::Setting &materialsSection = root["materials"];

    libconfig::Setting &materialSection = materialsSection.add(libconfig::Setting::TypeGroup);

    this->registerString("name", material->getName(), materialSection);
    this->registerString("model", material->getModelName(), materialSection);

    for (const auto &property : material->getProperties())
    {
        const std::string &name = property.first;
        float value = property.second;

        this->registerFloat(name, value, materialSection);
    }

    this->registerColor("base_color", material->getBaseColor().toColor(), materialSection);
    this->registerColor("specular", material->getSpecular().toColor(), materialSection);
}

void rc::SceneRegister::saveToFile(const std::string &file_path)
{
    try
    {
        this->_cfg.writeFile(file_path.c_str());
    }
    catch(const libconfig::FileIOException& e)
    {
        std::cerr << "Cannot write to file: " << file_path << std::endl;
    }   
}

void rc::SceneRegister::serializeScene(IScene *scene)
{
    libconfig::Setting &root = this->getRoot();

    root.add("environment", libconfig::Setting::TypeGroup);

    this->registerFloat("ambient", scene->getAmbientCoefficient(), root["environment"]);
    this->registerFloat("diffuse", scene->getDiffuseCoefficient(), root["environment"]);

    this->registerCamera(&scene->getCamera());

    root.add("objects", libconfig::Setting::TypeList);

    for (const auto &primitive : scene->getPrimitives())
        this->registerPrimitive(primitive);

    for (const auto &light : scene->getLights())
        this->registerLight(light);

    root.add("materials", libconfig::Setting::TypeList);

    for (const auto &entry : scene->getMaterials())
        this->registerMaterial(&entry.second);
}

void rc::SceneRegister::saveScene(const std::string &scene_path, IScene *scene)
{
    this->serializeScene(scene);
    this->saveToFile(scene_path);
}

/**
 * fonction de con permettant de transformer une scene en string parsable par notre lib
 * @param scene Scene a recuperer
 * @return La scene au format lisible par la config++
 */
std::string rc::SceneRegister::toString(IScene *scene)
{
    static const std::string tmpName = ".tmp_config_1";

    this->serializeScene(scene);
    this->saveToFile(tmpName);

    std::ifstream tmp(tmpName, std::ios::in);

    if (!tmp.is_open())
        return ("");

    std::ostringstream oss;

    oss << tmp.rdbuf();

    std::filesystem::remove(tmpName);

    return (oss.str());
}
