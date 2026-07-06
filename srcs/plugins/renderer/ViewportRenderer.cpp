//
// Created by jazema on 5/4/26.
//

#include "ViewportRenderer.hpp"

#include <algorithm>
#include <atomic>
#include <cmath>
#include <limits>
#include <thread>
#include <unordered_set>
#include <vector>

#include "../../common/Intersection.hpp"
#include "../../common/scene/ILight.hpp"

namespace
{
    constexpr int TILE_SIZE = 32;
    constexpr int DRAFT_STEP = 2;

    // --- Viewport studio lighting -------------------------------------------
    const rc::Vector3f KEY_DIR  = rc::normalize(rc::Vector3f(-0.35f, 0.75f, 0.55f));
    const rc::Vector3f FILL_DIR = rc::normalize(rc::Vector3f(0.72f, 0.10f, 0.30f));
    const rc::Vector3f BACK_DIR = rc::normalize(rc::Vector3f(0.20f, 0.35f, -0.90f));

    const rc::ColorF KEY_COLOR {1.00f, 0.94f, 0.82f};   // warm sun
    const rc::ColorF FILL_COLOR{0.30f, 0.38f, 0.52f};   // cool bounce
    const rc::ColorF RIM_COLOR {0.70f, 0.82f, 1.00f};   // cool rim / sky wrap

    const rc::ColorF SKY_AMBIENT   {0.42f, 0.52f, 0.68f}; // hemisphere: from above
    const rc::ColorF GROUND_AMBIENT{0.24f, 0.20f, 0.16f}; // hemisphere: from below

    // Background / sky gradient (also the ray-miss colour).
    const rc::ColorF SKY_ZENITH {0.20f, 0.36f, 0.58f};
    const rc::ColorF SKY_HORIZON{0.58f, 0.64f, 0.72f};
    const rc::ColorF SKY_NADIR  {0.05f, 0.07f, 0.11f};
    const rc::Color SELECTION_OUTLINE_COLOR{255, 200, 40};
    const rc::Color HOVER_OUTLINE_COLOR{120, 200, 255};
    constexpr int SELECTION_OUTLINE_THICKNESS = 2;
    constexpr int SELECTION_OUTLINE_GLOW = 5;
    constexpr int HOVER_OUTLINE_THICKNESS = 1;
    constexpr int HOVER_OUTLINE_GLOW = 3;
    const rc::Color LIGHT_GIZMO_COLOR{190, 190, 190};
    const rc::Color LIGHT_SELECTED_COLOR{255, 200, 40};
    const rc::Color LIGHT_HOVER_COLOR{170, 220, 255};

    struct ViewportHit
    {
        rc::Color color;
        const rc::IPrimitive *primitive = nullptr;
        bool hit = false;
    };

    struct ViewportCameraData
    {
        rc::Vector3f origin;
        rc::Vector3f pixel00;
        rc::Vector3f pixel_delta_u;
        rc::Vector3f pixel_delta_v;
    };

    ViewportCameraData build_viewport_camera_data(const rc::ICamera &camera, int width, int height)
    {
        const float focal_length = 1.0f;
        const float theta = static_cast<float>(camera.getFov()) * (3.14159265358979323846f / 180.0f);
        const float viewport_height = 2.0f * std::tan(theta / 2.0f);
        const float viewport_width = viewport_height * (static_cast<float>(width) / static_cast<float>(height));

        const rc::Vector3f origin = camera.getPosition();
        const rc::Vector3f forward = camera.getForward();
        const rc::Vector3f right = camera.getRight();
        rc::Vector3f up = {
            right.y * forward.z - right.z * forward.y,
            right.z * forward.x - right.x * forward.z,
            right.x * forward.y - right.y * forward.x
        };
        up = rc::normalize(up);

        const rc::Vector3f viewport_u = viewport_width * right;
        const rc::Vector3f viewport_v = -viewport_height * up;

        const rc::Vector3f pixel_delta_u = viewport_u / static_cast<float>(width);
        const rc::Vector3f pixel_delta_v = viewport_v / static_cast<float>(height);

        const rc::Vector3f viewport_upper_left = origin + focal_length * forward - viewport_u / 2.0f - viewport_v / 2.0f;
        const rc::Vector3f pixel00 = viewport_upper_left + 0.5f * (pixel_delta_u + pixel_delta_v);

        return {origin, pixel00, pixel_delta_u, pixel_delta_v};
    }

    rc::Ray viewport_primary_ray(const ViewportCameraData &camera_data, float x, float y)
    {
        const rc::Vector3f pixel_sample = camera_data.pixel00
                                         + (x * camera_data.pixel_delta_u)
                                         + (y * camera_data.pixel_delta_v);
        const rc::Vector3f ray_dir = (pixel_sample - camera_data.origin).unit_vector();
        return {camera_data.origin, ray_dir};
    }

    inline float saturate(float v)
    {
        return std::clamp(v, 0.0f, 1.0f);
    }

    inline rc::ColorF lerp(const rc::ColorF &a, const rc::ColorF &b, float t)
    {
        return a * (1.0f - t) + b * t;
    }

    inline float aces(float x)
    {
        const float a = 2.51f, b = 0.03f, c = 2.43f, d = 0.59f, e = 0.14f;
        return saturate((x * (a * x + b)) / (x * (c * x + d) + e));
    }

    inline rc::ColorF tonemap(const rc::ColorF &c)
    {
        return {aces(c.r), aces(c.g), aces(c.b)};
    }

    constexpr int AA_SAMPLES = 8;
    const rc::Vector2f AA_OFFSETS[AA_SAMPLES] = {
        {-0.375f, -0.125f}, {0.125f, -0.375f}, {0.375f, 0.125f}, {-0.125f, 0.375f},
        {-0.375f, 0.375f}, {0.375f, -0.375f}, {-0.125f, -0.125f}, {0.125f, 0.125f}
    };
    constexpr float AA_COLOR_THRESHOLD = 0.10f;

    inline float color_distance(const rc::Color &a, const rc::Color &b)
    {
        return (std::abs(a.r - b.r) + std::abs(a.g - b.g) + std::abs(a.b - b.b)) / 255.0f;
    }

    struct ShadeParams
    {
        float ambient;
        float diffuse;
    };

    rc::ColorF viewport_background(const rc::Ray &ray)
    {
        const float t = saturate(0.5f * (ray.direction.y + 1.0f));
        rc::ColorF sky = (t < 0.5f)
            ? lerp(SKY_NADIR, SKY_HORIZON, t * 2.0f)
            : lerp(SKY_HORIZON, SKY_ZENITH, (t - 0.5f) * 2.0f);

        const float sun = saturate(rc::dot(ray.direction, KEY_DIR));
        return sky + KEY_COLOR * (0.18f * std::pow(sun, 8.0f));
    }

    rc::ColorF viewport_shade(const rc::Intersection &hit, const rc::Vector3f &view_dir, const ShadeParams &shading)
    {
        const rc::Vector3f &N = hit.normal;
        const rc::Vector3f V = view_dir * -1.0f;            // toward the eye (unit)
        const rc::Material &mat = hit.material;
        const rc::ColorF albedo = mat.baseColor;

        const float up = 0.5f * (N.y + 1.0f);
        const rc::ColorF ambient = lerp(GROUND_AMBIENT, SKY_AMBIENT, up);

        const float key_d  = std::max(0.0f, rc::dot(N, KEY_DIR));
        const float fill_d = 0.5f * rc::dot(N, FILL_DIR) + 0.5f;
        const float back_d = std::max(0.0f, rc::dot(N, BACK_DIR));
        const rc::ColorF diffuse = KEY_COLOR * key_d
                                 + FILL_COLOR * (fill_d * fill_d * 0.7f)
                                 + RIM_COLOR * (back_d * 0.15f);

        const float ndv = std::max(0.0f, rc::dot(N, V));
        float spec = 0.0f;
        if (key_d > 0.0f)
        {
            const rc::Vector3f H = rc::normalize(KEY_DIR + V);
            const float ndh = std::max(0.0f, rc::dot(N, H));
            const float gloss = std::clamp(mat.shininess, 8.0f, 400.0f);
            const float energy = std::min(6.0f, 1.0f + gloss * 0.04f);
            spec = std::pow(ndh, gloss) * energy * key_d;
        }
        const float spec_k = saturate(mat.specular_level * (1.0f - 0.7f * mat.roughness) + mat.metallic);
        const rc::ColorF spec_color = lerp(rc::ColorF{1.0f, 1.0f, 1.0f}, albedo, mat.metallic) * (spec * spec_k);

        const float fresnel = std::pow(1.0f - ndv, 3.0f);
        const rc::ColorF rim = RIM_COLOR * (fresnel * (0.25f + 0.35f * up));

        const float amb_k = 0.30f + 0.70f * saturate(shading.ambient);
        const float dif_k = 0.40f + 0.60f * saturate(shading.diffuse);
        rc::ColorF lit = albedo * (ambient * amb_k + diffuse * dif_k) + spec_color + rim;
        lit = lit * (0.25f + 0.75f * saturate(mat.ao));

        return tonemap(lit);
    }

    rc::ColorF ray_color(const rc::Ray &ray, const rc::IScene &scene, const ShadeParams &shading)
    {
        rc::Intersection hit;

        if (!scene.intersect(ray, 0.001f, std::numeric_limits<float>::infinity(), hit))
            return viewport_background(ray);
        return viewport_shade(hit, ray.direction, shading);
    }

    ViewportHit ray_hit(const rc::Ray &ray, const rc::IScene &scene, const ShadeParams &shading)
    {
        rc::Intersection hit;

        if (!scene.intersect(ray, 0.001f, std::numeric_limits<float>::infinity(), hit))
            return {viewport_background(ray).toColor(), nullptr, false};
        return {viewport_shade(hit, ray.direction, shading).toColor(), hit.primitive, true};
    }

    void blend_pixel(rc::Color &dst, const rc::Color &src, float alpha)
    {
        alpha = std::clamp(alpha, 0.0f, 1.0f);
        const float inv = 1.0f - alpha;
        dst.r = static_cast<uint8_t>(std::lround(dst.r * inv + src.r * alpha));
        dst.g = static_cast<uint8_t>(std::lround(dst.g * inv + src.g * alpha));
        dst.b = static_cast<uint8_t>(std::lround(dst.b * inv + src.b * alpha));
    }

    void apply_outline_glow(rc::Render &render, const std::vector<uint8_t> &mask, rc::Color color, int thickness, int glow_radius)
    {
        if (render.size_x <= 0 || render.size_y <= 0)
            return;
        const size_t expected = static_cast<size_t>(render.size_x) * static_cast<size_t>(render.size_y);
        if (mask.size() != expected || render.pixels.size() != expected)
            return;

        const int w = render.size_x;
        const int h = render.size_y;
        const int radius = std::max(1, std::max(thickness, glow_radius));
        const int radius_sq = radius * radius;
        const float core = static_cast<float>(thickness);
        const float falloff = std::max(1.0f, static_cast<float>(glow_radius) - core);

        std::vector<float> alpha(expected, 0.0f);
        bool has_edge = false;

        for (int y = 0; y < h; ++y)
        {
            for (int x = 0; x < w; ++x)
            {
                const size_t idx = static_cast<size_t>(y) * static_cast<size_t>(w) + static_cast<size_t>(x);
                if (!mask[idx])
                    continue;

                const bool boundary =
                    x == 0 || !mask[idx - 1] ||
                    x == w - 1 || !mask[idx + 1] ||
                    y == 0 || !mask[idx - static_cast<size_t>(w)] ||
                    y == h - 1 || !mask[idx + static_cast<size_t>(w)];
                if (!boundary)
                    continue;
                has_edge = true;

                for (int dy = -radius; dy <= radius; ++dy)
                {
                    const int ny = y + dy;
                    if (ny < 0 || ny >= h)
                        continue;
                    for (int dx = -radius; dx <= radius; ++dx)
                    {
                        const int dist_sq = dx * dx + dy * dy;
                        if (dist_sq == 0 || dist_sq > radius_sq)
                            continue;
                        const int nx = x + dx;
                        if (nx < 0 || nx >= w)
                            continue;
                        const size_t nidx = static_cast<size_t>(ny) * static_cast<size_t>(w) + static_cast<size_t>(nx);
                        if (mask[nidx])
                            continue;

                        const float dist = std::sqrt(static_cast<float>(dist_sq));
                        float a;
                        if (dist <= core)
                            a = 1.0f;
                        else
                        {
                            const float f = 1.0f - (dist - core) / falloff;
                            a = f * f;
                        }
                        if (a > alpha[nidx])
                            alpha[nidx] = a;
                    }
                }
            }
        }

        if (!has_edge)
            return;

        for (size_t idx = 0; idx < expected; ++idx)
        {
            if (alpha[idx] > 0.0f)
                blend_pixel(render.pixels[idx], color, alpha[idx]);
        }
    }

    bool project_to_pixel(const rc::ICamera &camera, const rc::Vector3f &point, int width, int height, rc::Vector2i &pixel)
    {
        if (width <= 0 || height <= 0)
            return false;

        const rc::Vector3f cam_pos = camera.getPosition();
        const rc::Vector3f forward = camera.getForward();
        rc::Vector3f right = camera.getRight();
        rc::Vector3f up = right.cross(forward);
        up = rc::normalize(up);

        const rc::Vector3f to_point = point - cam_pos;
        const float z = rc::dot(to_point, forward);
        if (z <= 0.001f)
            return false;

        const float x = rc::dot(to_point, right);
        const float y = rc::dot(to_point, up);

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

    void set_render_pixel(rc::Render &render, int x, int y, rc::Color color)
    {
        if (x < 0 || y < 0 || x >= render.size_x || y >= render.size_y)
            return;
        const size_t idx = static_cast<size_t>(y) * static_cast<size_t>(render.size_x) + static_cast<size_t>(x);
        if (idx >= render.pixels.size())
            return;
        render.pixels[idx] = color;
    }

    void draw_circle(rc::Render &render, const rc::Vector2i &center, int radius, rc::Color color)
    {
        if (radius <= 0)
            return;
        const int steps = std::max(24, radius * 6);
        const float step = 2.0f * 3.14159265358979323846f / static_cast<float>(steps);
        for (int i = 0; i < steps; ++i)
        {
            const float a = step * static_cast<float>(i);
            const int x = center.x + static_cast<int>(std::cos(a) * radius);
            const int y = center.y + static_cast<int>(std::sin(a) * radius);
            set_render_pixel(render, x, y, color);
        }
    }

    void draw_line(rc::Render &render, const rc::Vector2i &start, const rc::Vector2i &end, rc::Color color)
    {
        int x0 = start.x;
        int y0 = start.y;
        int x1 = end.x;
        int y1 = end.y;

        int dx = std::abs(x1 - x0);
        int dy = std::abs(y1 - y0);
        int sx = (x0 < x1) ? 1 : -1;
        int sy = (y0 < y1) ? 1 : -1;
        int err = dx - dy;

        while (true)
        {
            set_render_pixel(render, x0, y0, color);
            if (x0 == x1 && y0 == y1)
                break;
            int e2 = 2 * err;
            if (e2 > -dy)
            {
                err -= dy;
                x0 += sx;
            }
            if (e2 < dx)
            {
                err += dx;
                y0 += sy;
            }
        }
    }

    rc::Color light_gizmo_color(const rc::ILight *light, bool selected, bool hovered)
    {
        if (selected)
            return LIGHT_SELECTED_COLOR;
        if (hovered)
            return LIGHT_HOVER_COLOR;
        if (!light)
            return LIGHT_GIZMO_COLOR;
        rc::Color c = light->getColorF().toColor();
        auto boost = [](uint8_t v) -> uint8_t
        {
            int value = static_cast<int>(v) + 60;
            return static_cast<uint8_t>(std::min(255, value));
        };
        return rc::Color(boost(c.r), boost(c.g), boost(c.b));
    }

    void draw_light_gizmos(rc::Render &render, const rc::IScene &scene, const rc::ICamera &camera, const std::unordered_set<const rc::ISceneObject *> &selection, const rc::ISceneObject *hover)
    {
        const auto &lights = scene.getLights();
        if (lights.empty())
            return;

        const int radius = 6;
        for (const auto *light : lights)
        {
            if (!light)
                continue;
            if (light->isHidden())
                continue;

            rc::Vector3f position = light->getPosition();
            if (light->getKind() == rc::LightKind::DIRECTIONAL)
            {
                rc::Vector3f dir = rc::normalize(light->getRotation());
                position = camera.getPosition() + dir * 5.0f;
            }

            rc::Vector2i center;
            if (!project_to_pixel(camera, position, render.size_x, render.size_y, center))
                continue;

            const bool is_selected = selection.count(light) > 0;
            const bool is_hovered = !is_selected && hover == light;
            rc::Color color = light_gizmo_color(light, is_selected, is_hovered);
            draw_circle(render, center, radius, color);

            if (light->getKind() == rc::LightKind::DIRECTIONAL)
            {
                rc::Vector3f dir = rc::normalize(light->getRotation());
                rc::Vector2i tip;
                if (project_to_pixel(camera, position + dir * 1.2f, render.size_x, render.size_y, tip))
                    draw_line(render, center, tip, color);
            }
        }
    }
}

namespace rc
{
    bool ViewportRenderer::needsGeometryRefresh(const IScene &scene) const
    {
        const ICamera &camera = scene.getCamera();
        const Vector2i resolution = camera.getResolution();

        std::lock_guard<std::mutex> lock(_cacheMutex);
        if (this->_sceneDirty)
            return (true);
        if (this->_lastScene != &scene)
            return (true);
        if (this->_lastResolution.x != resolution.x || this->_lastResolution.y != resolution.y)
            return (true);
        if (this->_lastCameraPosition.x != camera.getPosition().x || this->_lastCameraPosition.y != camera.getPosition().y || this->_lastCameraPosition.z != camera.getPosition().z)
            return (true);
        if (this->_lastCameraRotation.x != camera.getRotation().x || this->_lastCameraRotation.y != camera.getRotation().y || this->_lastCameraRotation.z != camera.getRotation().z)
            return (true);
        if (this->_lastCameraFov != camera.getFov())
            return (true);
        if (this->_lastSamplesPerPixel != camera.getSamplesPerPixel())
            return (true);
        return (false);
    }

    void ViewportRenderer::renderScene(const IScene &scene)
    {
        const ICamera &camera = scene.getCamera();
        const Vector2i resolution = camera.getResolution();

        if (resolution.x <= 0 || resolution.y <= 0)
            return;

        const size_t pixel_count = static_cast<size_t>(resolution.x) * static_cast<size_t>(resolution.y);

        std::vector<const ISceneObject *> selection;
        size_t selection_version = 0;
        const ISceneObject *hover = nullptr;
        size_t hover_version = 0;
        bool selection_changed = false;
        bool hover_changed = false;
        {
            std::lock_guard lock(this->_cacheMutex);
            selection = this->_selection;
            selection_version = this->_selectionVersion;
            hover = this->_hover;
            hover_version = this->_hoverVersion;
            selection_changed = this->_lastSelectionVersion != selection_version;
            hover_changed = this->_lastHoverVersion != hover_version;
        }

        const bool camera_moved = this->needsGeometryRefresh(scene);
        bool base_stale = false;
        bool pending_refine = false;
        bool pending_aa = false;
        {
            std::lock_guard lock(_renderMutex);
            base_stale = this->_baseColors.size() != pixel_count;
        }
        {
            std::lock_guard lock(_cacheMutex);
            pending_refine = this->_pendingRefine;
            pending_aa = this->_pendingAA;
        }
        const bool geometry_dirty = camera_moved || selection_changed || base_stale || pending_refine || pending_aa;

        if (!geometry_dirty && !hover_changed)
            return;

        enum class Quality { Draft, Full, Aa };
        Quality quality;
        if (camera_moved && !base_stale)
            quality = Quality::Draft;
        else if (pending_aa && !pending_refine && !camera_moved && !selection_changed && !base_stale)
            quality = Quality::Aa;
        else
            quality = Quality::Full;
        const bool draft = quality == Quality::Draft;

        this->_rendering = true;
        this->_stopRequested = false;

        if (geometry_dirty)
        {
            const ViewportCameraData camera_data = build_viewport_camera_data(camera, resolution.x, resolution.y);
            const ShadeParams shading{scene.getAmbientCoefficient(), scene.getDiffuseCoefficient()};

            const int tiles_x = (resolution.x + TILE_SIZE - 1) / TILE_SIZE;
            const int tiles_y = (resolution.y + TILE_SIZE - 1) / TILE_SIZE;
            const int total_tiles = tiles_x * tiles_y;
            const size_t thread_count = std::max<size_t>(1, std::thread::hardware_concurrency());
            auto run_tiles = [&](auto &&tile_fn)
            {
                std::atomic<int> next_tile = 0;
                auto worker = [&]
                {
                    while (true)
                    {
                        if (this->_stopRequested)
                            break;
                        const int tile = next_tile++;
                        if (tile >= total_tiles)
                            break;
                        tile_fn(tile % tiles_x, tile / tiles_x);
                    }
                };
                std::vector<std::thread> workers;
                workers.reserve(thread_count);
                for (size_t i = 0; i < thread_count; ++i)
                    workers.emplace_back(worker);
                for (auto &worker_thread : workers)
                    worker_thread.join();
            };

            if (quality == Quality::Aa)
            {
                std::vector<Color> base;
                std::vector<const IPrimitive *> ids;
                {
                    std::lock_guard lock(_renderMutex);
                    base = this->_baseColors;
                    ids = this->_primitiveIds;
                }
                if (base.size() == pixel_count && ids.size() == pixel_count)
                {
                    std::vector<Color> smoothed = base;
                    const int w = resolution.x;
                    const int h = resolution.y;
                    auto aa_tile = [&](int tile_x, int tile_y)
                    {
                        static const int dxs[4] = {-1, 1, 0, 0};
                        static const int dys[4] = {0, 0, -1, 1};
                        const int start_x = tile_x * TILE_SIZE;
                        const int start_y = tile_y * TILE_SIZE;
                        const int end_x = std::min(start_x + TILE_SIZE, w);
                        const int end_y = std::min(start_y + TILE_SIZE, h);
                        for (int y = start_y; y < end_y; ++y)
                        {
                            if (this->_stopRequested)
                                return;
                            const size_t row = static_cast<size_t>(y) * static_cast<size_t>(w);
                            for (int x = start_x; x < end_x; ++x)
                            {
                                const size_t idx = row + static_cast<size_t>(x);
                                const IPrimitive *id = ids[idx];
                                bool edge = false;
                                for (int k = 0; k < 4 && !edge; ++k)
                                {
                                    const int nx = x + dxs[k];
                                    const int ny = y + dys[k];
                                    if (nx < 0 || ny < 0 || nx >= w || ny >= h)
                                        continue;
                                    const size_t nidx = static_cast<size_t>(ny) * static_cast<size_t>(w) + static_cast<size_t>(nx);
                                    if (ids[nidx] != id || color_distance(base[idx], base[nidx]) > AA_COLOR_THRESHOLD)
                                        edge = true;
                                }
                                if (!edge)
                                    continue;

                                ColorF acc{0.0f, 0.0f, 0.0f};
                                for (int s = 0; s < AA_SAMPLES; ++s)
                                {
                                    const Ray ray = viewport_primary_ray(camera_data,
                                        static_cast<float>(x) + AA_OFFSETS[s].x,
                                        static_cast<float>(y) + AA_OFFSETS[s].y);
                                    acc = acc + ray_color(ray, scene, shading);
                                }
                                smoothed[idx] = (acc * (1.0f / static_cast<float>(AA_SAMPLES))).toColor();
                            }
                        }
                    };
                    run_tiles(aa_tile);

                    if (!this->_stopRequested)
                    {
                        std::lock_guard lock(_renderMutex);
                        this->_baseColors = std::move(smoothed);
                    }
                }
            }
            else
            {
                const int step = draft ? DRAFT_STEP : 1;
                std::vector<Color> frame_buffer(pixel_count, Color());
                std::vector<const IPrimitive *> id_buffer(pixel_count, nullptr);

                auto render_tile = [&](int tile_x, int tile_y)
                {
                    const int start_x = tile_x * TILE_SIZE;
                    const int start_y = tile_y * TILE_SIZE;
                    const int end_x = std::min(start_x + TILE_SIZE, resolution.x);
                    const int end_y = std::min(start_y + TILE_SIZE, resolution.y);

                    for (int by = start_y; by < end_y; by += step)
                    {
                        if (this->_stopRequested)
                            return;
                        for (int bx = start_x; bx < end_x; bx += step)
                        {
                            const int block_end_x = std::min(bx + step, end_x);
                            const int block_end_y = std::min(by + step, end_y);
                            const int sample_x = std::min(bx + step / 2, block_end_x - 1);
                            const int sample_y = std::min(by + step / 2, block_end_y - 1);

                            const Ray ray = viewport_primary_ray(camera_data, sample_x, sample_y);
                            const ViewportHit hit = ray_hit(ray, scene, shading);
                            const IPrimitive *id = hit.hit ? hit.primitive : nullptr;

                            for (int y = by; y < block_end_y; ++y)
                            {
                                const size_t row = static_cast<size_t>(y) * static_cast<size_t>(resolution.x);
                                for (int x = bx; x < block_end_x; ++x)
                                {
                                    frame_buffer[row + static_cast<size_t>(x)] = hit.color;
                                    id_buffer[row + static_cast<size_t>(x)] = id;
                                }
                            }
                        }
                    }
                };

                run_tiles(render_tile);

                if (this->_stopRequested)
                {
                    this->_rendering = false;
                    return;
                }

                std::lock_guard lock(_renderMutex);
                this->_baseColors = std::move(frame_buffer);
                this->_primitiveIds = std::move(id_buffer);
            }
        }

        std::vector<Color> composite;
        std::vector<const IPrimitive *> ids;
        {
            std::lock_guard lock(_renderMutex);
            composite = this->_baseColors;
            ids = this->_primitiveIds;
        }
        if (composite.size() != pixel_count || ids.size() != pixel_count)
        {
            this->_rendering = false;
            return;
        }

        const std::unordered_set selection_set(selection.begin(), selection.end());
        const bool has_selection = !selection_set.empty();
        const bool has_hover = hover != nullptr && selection_set.count(hover) == 0;
        std::vector<uint8_t> selection_mask(pixel_count, 0);
        std::vector<uint8_t> hover_mask(pixel_count, 0);
        for (size_t idx = 0; idx < pixel_count; ++idx)
        {
            const IPrimitive *primitive = ids[idx];
            if (!primitive)
                continue;
            if (has_selection && selection_set.count(primitive) > 0)
                selection_mask[idx] = 1;
            else if (has_hover && primitive == hover)
                hover_mask[idx] = 1;
        }

        Render final_render = {resolution.x, resolution.y, std::move(composite)};

        if (has_hover)
            apply_outline_glow(final_render, hover_mask, HOVER_OUTLINE_COLOR, HOVER_OUTLINE_THICKNESS, HOVER_OUTLINE_GLOW);
        if (has_selection)
            apply_outline_glow(final_render, selection_mask, SELECTION_OUTLINE_COLOR, SELECTION_OUTLINE_THICKNESS, SELECTION_OUTLINE_GLOW);

        draw_light_gizmos(final_render, scene, camera, selection_set, hover);

        {
            std::lock_guard lock(_renderMutex);
            this->_render = std::move(final_render);
        }

        {
            std::lock_guard lock(_cacheMutex);
            this->_sceneDirty = false;
            this->_lastScene = &scene;
            this->_lastResolution = resolution;
            this->_lastCameraPosition = camera.getPosition();
            this->_lastCameraRotation = camera.getRotation();
            this->_lastCameraFov = camera.getFov();
            this->_lastSamplesPerPixel = camera.getSamplesPerPixel();
            this->_lastSelectionVersion = selection_version;
            this->_lastHoverVersion = hover_version;
            this->_pendingRefine = (quality == Quality::Draft);
            this->_pendingAA = (quality == Quality::Full);
        }

        this->_rendering = false;
    }

    void ViewportRenderer::stopRendering()
    {
        this->_stopRequested = true;
    }

    void ViewportRenderer::setPixel(int x, int y, Color color)
    {
        std::lock_guard lock(_renderMutex);
        if (x < 0 || y < 0 || x >= this->_render.size_x || y >= this->_render.size_y)
            return;
        this->_render.pixels[y * this->_render.size_x + x] = color;
    }

    void ViewportRenderer::markSceneDirty()
    {
        std::lock_guard lock(this->_cacheMutex);
        this->_sceneDirty = true;
    }

    void ViewportRenderer::setSelection(const std::vector<const ISceneObject *> &selection)
    {
        std::lock_guard lock(this->_cacheMutex);
        this->_selection = selection;
        this->_selectionVersion++;
    }

    void ViewportRenderer::setHover(const ISceneObject *object)
    {
        std::lock_guard lock(this->_cacheMutex);
        if (this->_hover == object)
            return;
        this->_hover = object;
        this->_hoverVersion++;
    }

    std::string ViewportRenderer::getRendererName() const
    {
        return ("Viewport");
    }

    bool ViewportRenderer::isRendering() const
    {
        return (this->_rendering);
    }

    int ViewportRenderer::getCurrentSample() const
    {
        return (-1);
    }

    Render ViewportRenderer::getRender() const
    {
        std::lock_guard<std::mutex> lock(_renderMutex);
        return (this->_render);
    }

    PluginType ViewportRenderer::getType() const
    {
        return (PluginType::RENDERER);
    }

    extern "C" IPlugin *create_plugin()
    {
        return (new ViewportRenderer());
    }

    extern "C" void destroy_plugin(IPlugin* plugin)
    {
        delete (plugin);
    }
}
