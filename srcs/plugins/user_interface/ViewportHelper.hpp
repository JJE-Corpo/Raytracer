//
// Created by jazema on 5/10/26.
//

#ifndef VIEWPORTHELPER_HPP
#define VIEWPORTHELPER_HPP
#include <array>
#include <string>
#include <SFML/System/Vector2.hpp>

#include "../../common/scene/ICamera.hpp"
#include "../../common/scene/ILight.hpp"
#include "../../common/Material.hpp"
#include "../../common/Vector.hpp"

namespace rc
{
    constexpr int                         LIGHT_GIZMO_PICK_RADIUS   = 9;
    constexpr std::array<const char *, 4> PHONG_MATERIAL_FLOAT_KEYS = {
        "shininess",
        "reflectivity",
        "transparency",
        "ior",
        //"normal_map_enabled",
        //"normal_scale",
        //"normal_noise_frequency"
    };

    constexpr std::array<const char *, 11> PBR_MATERIAL_FLOAT_KEYS = {
        "metallic",
        "roughness",
        "ao",
        "specular_level",
        "specular_tint",
        "clearcoat",
        "clearcoat_roughness",
        "sheen",
        "sheen_tint",
        "transmission",
        "alpha",
        //"normal_map_enabled",
        //"normal_scale",
        //"normal_noise_frequency"
    };

    class ViewportHelper
    {
        public:
            template<std::size_t N>
            static bool hasMaterialKey(const std::string &name, const std::array<const char *, N> &keys)
            {
                for (const char *key: keys)
                {
                    if (name == key)
                        return true;
                }

                return false;
            }

            static bool isMaterialFloatSlider(const std::string &name, MaterialModel model)
            {
                return model == MaterialModel::PHONG
                           ? hasMaterialKey(name, PHONG_MATERIAL_FLOAT_KEYS)
                           : hasMaterialKey(name, PBR_MATERIAL_FLOAT_KEYS);
            }

            static bool projectToPixel(const ICamera &camera, const Vector3f &point, int width, int height,
                                sf::Vector2i & pixel)
            {
                if (width <= 0 || height <= 0)
                    return false;

                const Vector3f cam_pos = camera.getPosition();
                const Vector3f forward = camera.getForward();
                Vector3f       right   = camera.getRight();
                Vector3f       up      = right.cross(forward);
                up                     = normalize(up);

                const Vector3f to_point = point - cam_pos;
                const float    z        = dot(to_point, forward);
                if (z <= 0.001f)
                    return false;

                const float x = dot(to_point, right);
                const float y = dot(to_point, up);

                const float theta = static_cast<float>(camera.getFov()) * (3.14159265358979323846f / 180.0f);
                const float viewport_height = 2.0f * std::tan(theta / 2.0f);
                const float viewport_width = viewport_height * (static_cast<float>(width) / static_cast<float>(height));

                const float ndc_x = (x / z) / (viewport_width / 2.0f);
                const float ndc_y = (y / z) / (viewport_height / 2.0f);

                if (ndc_x < -1.0f || ndc_x > 1.0f || ndc_y < -1.0f || ndc_y > 1.0f)
                    return false;

                const float px = (ndc_x * 0.5f + 0.5f) * static_cast<float>(width - 1);
                const float py = (-ndc_y * 0.5f + 0.5f) * static_cast<float>(height - 1);

                pixel = {static_cast<int>(px + 0.5f), static_cast<int>(py + 0.5f)};
                return true;
            }

            static const ILight *pickViewportLight(const IScene &scene, const ICamera &camera, const sf::Vector2i &pixel)
            {
                const auto &   lights      = scene.getLights();
                const Vector2i render_size = camera.getResolution();
                const int      width       = render_size.x;
                const int      height      = render_size.y;

                const ILight *best_light       = nullptr;
                int           best_distance_sq = LIGHT_GIZMO_PICK_RADIUS * LIGHT_GIZMO_PICK_RADIUS;

                for (const ILight *light: lights)
                {
                    if (!light)
                        continue;
                    if (light->isHidden())
                        continue;

                    Vector3f position = light->getPosition();
                    if (light->getKind() == LightKind::DIRECTIONAL)
                        position = camera.getPosition() + normalize(light->getRotation()) * 5.0f;

                    sf::Vector2i center;
                    if (!projectToPixel(camera, position, width, height, center))
                        continue;

                    const int dx          = pixel.x - center.x;
                    const int dy          = pixel.y - center.y;
                    const int distance_sq = dx * dx + dy * dy;
                    if (distance_sq <= best_distance_sq)
                    {
                        best_distance_sq = distance_sq;
                        best_light       = light;
                    }
                }

                return (best_light);
            }
    };
}

#endif
