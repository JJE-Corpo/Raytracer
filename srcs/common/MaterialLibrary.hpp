//
// Created by the materials market feature.
//
// Shared, header-only persistence for the "materials market": every material
// created or loaded is written to ~/.raytracer/materials as a standalone JSON
// file, and the market UI reads that folder back to offer the materials for
// reuse in any scene. Kept header-only so both the core (which auto-saves on
// create/load) and the user_interface plugin (which browses the folder) can use
// it without extra link steps.
//

#ifndef MATERIALLIBRARY_HPP
#define MATERIALLIBRARY_HPP

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

#include <nlohmann/json.hpp>

#include "Material.hpp"

namespace rc
{
    class MaterialLibrary
    {
        public:
            // Absolute path to ~/.raytracer/materials, creating it on the way.
            // Returns "" when HOME is unset or the folder cannot be created, so
            // callers can silently skip persistence rather than crash.
            static std::string directory()
            {
                const char *home = std::getenv("HOME");

                if (home == nullptr || home[0] == '\0')
                    return ("");

                std::error_code ec;
                std::filesystem::path dir = std::filesystem::path(home) / ".raytracer" / "materials";
                std::filesystem::create_directories(dir, ec);
                if (ec)
                    return ("");
                return (dir.string());
            }

            // Write `material` to <dir>/<name>.json. A material with the same
            // name overwrites the previous file (names are the market's key).
            static bool save(const Material &material)
            {
                const std::string dir = directory();

                if (dir.empty())
                    return (false);

                std::filesystem::path path = std::filesystem::path(dir) / (sanitize(material.name) + ".json");
                std::ofstream file(path);

                if (!file.is_open())
                    return (false);
                file << toJson(material).dump(4) << std::endl;
                return (true);
            }

            // Every material found in the market folder, sorted by name so the
            // UI shows a stable order. Unreadable/invalid files are skipped.
            static std::vector<Material> loadAll()
            {
                std::vector<Material> materials;
                const std::string dir = directory();

                if (dir.empty())
                    return (materials);

                std::error_code ec;
                for (const auto &entry : std::filesystem::directory_iterator(dir, ec))
                {
                    if (ec)
                        break;
                    if (!entry.is_regular_file())
                        continue;
                    if (entry.path().extension() != ".json")
                        continue;

                    std::ifstream file(entry.path());
                    if (!file.is_open())
                        continue;
                    try
                    {
                        nlohmann::json object = nlohmann::json::parse(file);
                        materials.push_back(fromJson(object));
                    }
                    catch (const std::exception &)
                    {
                        continue;
                    }
                }
                std::sort(materials.begin(), materials.end(), [](const Material &a, const Material &b) {
                    return (a.name < b.name);
                });
                return (materials);
            }

        private:
            // Keep filenames tame: anything that is not alphanumeric, '-' or '_'
            // becomes '_'. Falls back to "material" for an otherwise empty name.
            static std::string sanitize(const std::string &name)
            {
                std::string result;

                for (char c : name)
                {
                    if (std::isalnum(static_cast<unsigned char>(c)) || c == '-' || c == '_')
                        result += c;
                    else
                        result += '_';
                }
                if (result.empty())
                    return ("material");
                return (result);
            }

            static nlohmann::json colorJson(const ColorF &color)
            {
                const Color c = color.toColor();
                return (nlohmann::json::array({static_cast<int>(c.r), static_cast<int>(c.g), static_cast<int>(c.b)}));
            }

            static ColorF readColor(const nlohmann::json &value, const ColorF &fallback)
            {
                if (!value.is_array() || value.size() != 3)
                    return (fallback);
                for (const auto &component : value)
                    if (!component.is_number())
                        return (fallback);
                Color c(static_cast<uint8_t>(value[0].get<int>()),
                        static_cast<uint8_t>(value[1].get<int>()),
                        static_cast<uint8_t>(value[2].get<int>()));
                return (ColorF::fromColor(c));
            }

            static float readFloat(const nlohmann::json &object, const char *key, float fallback)
            {
                if (object.contains(key) && object[key].is_number())
                    return (object[key].get<float>());
                return (fallback);
            }

            static nlohmann::json toJson(const Material &material)
            {
                nlohmann::json object;

                object["name"] = material.name;
                object["model"] = material.getModelName();
                object["base_color"] = colorJson(material.baseColor);
                object["specular"] = colorJson(material.specular);
                object["shininess"] = material.shininess;
                object["reflectivity"] = material.reflectivity;
                object["transparency"] = material.transparency;
                object["ior"] = material.ior;
                object["metallic"] = material.metallic;
                object["roughness"] = material.roughness;
                object["ao"] = material.ao;
                object["specular_level"] = material.specular_level;
                object["specular_tint"] = material.specular_tint;
                object["clearcoat"] = material.clearcoat;
                object["clearcoat_roughness"] = material.clearcoat_roughness;
                object["sheen"] = material.sheen;
                object["sheen_tint"] = material.sheen_tint;
                object["transmission"] = material.transmission;
                object["alpha"] = material.alpha;
                object["normal_map"] = material.normal_map;
                object["normal_map_enabled"] = material.normal_map_enabled;
                object["normal_scale"] = material.normal_scale;
                object["normal_noise_frequency"] = material.normal_noise_frequency;
                return (object);
            }

            static Material fromJson(const nlohmann::json &object)
            {
                Material material;

                if (object.contains("name") && object["name"].is_string())
                    material.name = object["name"].get<std::string>();
                if (object.contains("model") && object["model"].is_string())
                    material.model = (object["model"].get<std::string>() == "pbr") ? MaterialModel::PBR : MaterialModel::PHONG;
                if (object.contains("base_color"))
                    material.baseColor = readColor(object["base_color"], material.baseColor);
                if (object.contains("specular"))
                    material.specular = readColor(object["specular"], material.specular);

                material.shininess = readFloat(object, "shininess", material.shininess);
                material.reflectivity = readFloat(object, "reflectivity", material.reflectivity);
                material.transparency = readFloat(object, "transparency", material.transparency);
                material.ior = readFloat(object, "ior", material.ior);
                material.metallic = readFloat(object, "metallic", material.metallic);
                material.roughness = readFloat(object, "roughness", material.roughness);
                material.ao = readFloat(object, "ao", material.ao);
                material.specular_level = readFloat(object, "specular_level", material.specular_level);
                material.specular_tint = readFloat(object, "specular_tint", material.specular_tint);
                material.clearcoat = readFloat(object, "clearcoat", material.clearcoat);
                material.clearcoat_roughness = readFloat(object, "clearcoat_roughness", material.clearcoat_roughness);
                material.sheen = readFloat(object, "sheen", material.sheen);
                material.sheen_tint = readFloat(object, "sheen_tint", material.sheen_tint);
                material.transmission = readFloat(object, "transmission", material.transmission);
                material.alpha = readFloat(object, "alpha", material.alpha);
                material.normal_scale = readFloat(object, "normal_scale", material.normal_scale);
                material.normal_noise_frequency = readFloat(object, "normal_noise_frequency", material.normal_noise_frequency);

                if (object.contains("normal_map") && object["normal_map"].is_string())
                    material.normal_map = object["normal_map"].get<std::string>();
                if (object.contains("normal_map_enabled") && object["normal_map_enabled"].is_boolean())
                    material.normal_map_enabled = object["normal_map_enabled"].get<bool>();
                return (material);
            }
    };
}

#endif
