//
// Created by jazema on 5/7/26.
//

#ifndef MATERIAL_HPP
#define MATERIAL_HPP

#include <atomic>
#include <string>
#include <map>

#include "Color.hpp"

namespace rc
{
    enum class MaterialModel
    {
        PHONG,
        PBR
    };

    struct Material
    {
        MaterialModel model = MaterialModel::PHONG;
        std::string name = "material";
        ColorF baseColor = {0.9f, 0.9f, 0.9f};
        ColorF specular = {0.0f, 0.0f, 0.0f};
        float shininess = 32.0f;
        float reflectivity = 0.0f;
        float transparency = 0.0f;
        float ior = 1.5f;
        float metallic = 0.0f;
        float roughness = 0.5f;
        float ao = 1.0f;
        float specular_level = 0.5f;
        float specular_tint = 0.0f;
        float clearcoat = 0.0f;
        float clearcoat_roughness = 0.03f;
        float sheen = 0.0f;
        float sheen_tint = 0.5f;
        float transmission = 0.0f;
        float alpha = 1.0f;
        // Normal map settings
        std::string normal_map = ""; // path to normal map image (if any)
        bool normal_map_enabled = false;
        float normal_scale = 1.0f; // strength of the normal map effect (0..1)
        float normal_noise_frequency = 1.0f; // frequency for procedural normal noise
        // Albedo/base-color texture settings
        std::string texture_map = ""; // path to a PNG/JPG sampled as base color
        bool texture_map_enabled = false;
        float texture_uv_scale = 1.0f; // UV tiling factor (repeats per unit UV)

        template <typename T>
        void update(const std::string &key, T value)
        {
            if (key == "model")
                model = static_cast<MaterialModel>(value);
            // else if (key == "baseColor")
            //     baseColor = value;
            // else if (key == "specular")
            //     specular = value;
            else if (key == "shininess")
                shininess = value;
            else if (key == "reflectivity")
                reflectivity = value;
            else if (key == "transparency")
                transparency = value;
            else if (key == "ior")
                ior = value;
            else if (key == "metallic")
                metallic = value;
            else if (key == "roughness")
                roughness = value;
            else if (key == "ao")
                ao = value;
            else if (key == "specular_level")
                specular_level = value;
            else if (key == "specular_tint")
                specular_tint = value;
            else if (key == "clearcoat")
                clearcoat = value;
            else if (key == "clearcoat_roughness")
                clearcoat_roughness = value;
            else if (key == "sheen")
                sheen = value;
            else if (key == "sheen_tint")
                sheen_tint = value;
            else if (key == "transmission")
                transmission = value;
            else if (key == "alpha")
                alpha = value;
            else if (key == "normal_map")
                normal_map = value;
            else if (key == "normal_map_enabled")
                normal_map_enabled = value;
            else if (key == "normal_scale")
                normal_scale = value;
            else if (key == "normal_noise_frequency")
                normal_noise_frequency = value;
            else if (key == "texture_map")
                texture_map = value;
            else if (key == "texture_map_enabled")
                texture_map_enabled = value;
            else if (key == "texture_uv_scale")
                texture_uv_scale = value;
        }

        std::string getModelName() const
        {
            switch (this->model)
            {
                case MaterialModel::PHONG:
                    return "phong";
                case MaterialModel::PBR:
                    return "pbr";
                default:
                    return "unknown";
            }
        }

        std::string getName() const
        {
            return (name);
        }

        ColorF getBaseColor() const
        {
            return (this->baseColor);
        }

        ColorF getSpecular() const
        {
            return (specular);
        }

        std::map<std::string, float> getProperties() const
        {
            return {
                {"shininess", this->shininess},
                {"reflectivity", this->reflectivity},
                {"transparency", this->transparency},
                {"ior", this->ior},
                {"metallic", this->metallic},
                {"roughness", this->roughness},
                {"ao", this->ao},
                {"specular_level", this->specular_level},
                {"specular_tint", this->specular_tint},
                {"clearcoat", this->clearcoat},
                {"clearcoat_roughness", this->clearcoat_roughness},
                {"sheen", this->sheen},
                {"sheen_tint", this->sheen_tint},
                {"transmission", this->transmission},
                {"alpha", this->alpha},
                {"normal_map_enabled", this->normal_map_enabled ? 1.0f : 0.0f},
                {"normal_scale", this->normal_scale}
                ,{"normal_noise_frequency", this->normal_noise_frequency}
            };
        }

        std::string getNormalMap() const
        {
            return this->normal_map;
        }
    };
}

#endif
