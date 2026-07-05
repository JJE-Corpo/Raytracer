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
    const rc::Vector3f VIEWPORT_LIGHT = rc::normalize(rc::Vector3f(-0.35f, 0.75f, 0.55f));
    const rc::ColorF SKY_TOP{56.0f / 255.0f, 96.0f / 255.0f, 150.0f / 255.0f};
    const rc::ColorF SKY_BOTTOM{11.0f / 255.0f, 18.0f / 255.0f, 28.0f / 255.0f};
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

    rc::Ray viewport_primary_ray(const ViewportCameraData &camera_data, int x, int y)
    {
        const rc::Vector3f pixel_sample = camera_data.pixel00
                                         + (static_cast<float>(x) * camera_data.pixel_delta_u)
                                         + (static_cast<float>(y) * camera_data.pixel_delta_v);
        const rc::Vector3f ray_dir = (pixel_sample - camera_data.origin).unit_vector();
        return {camera_data.origin, ray_dir};
    }

    rc::ColorF viewport_background(const rc::Ray &ray)
    {
        rc::Vector3f dir = ray.direction.unit_vector();
        float t = 0.5f * (dir.y + 1.0f);

        return (SKY_BOTTOM * (1.0f - t) + SKY_TOP * t);
    }

    rc::ColorF viewport_shade(const rc::Intersection &hit, const rc::IScene &scene)
    {
        float lambert = std::max(0.0f, rc::dot(hit.normal, VIEWPORT_LIGHT));
        float shade = scene.getAmbientCoefficient() + scene.getDiffuseCoefficient() * (0.25f + 0.75f * lambert);

        shade = std::clamp(shade, 0.0f, 1.0f);
        return (hit.material.baseColor * shade);
    }

    ViewportHit ray_hit(const rc::Ray &ray, const rc::IScene &scene)
    {
        rc::Intersection hit;

        if (!scene.intersect(ray, 0.001f, std::numeric_limits<float>::infinity(), hit))
            return {viewport_background(ray).toColor(), nullptr, false};
        return {viewport_shade(hit, scene).toColor(), hit.primitive, true};
    }

    void blend_pixel(rc::Color &dst, const rc::Color &src, float alpha)
    {
        alpha = std::clamp(alpha, 0.0f, 1.0f);
        const float inv = 1.0f - alpha;
        dst.r = static_cast<uint8_t>(std::lround(dst.r * inv + src.r * alpha));
        dst.g = static_cast<uint8_t>(std::lround(dst.g * inv + src.g * alpha));
        dst.b = static_cast<uint8_t>(std::lround(dst.b * inv + src.b * alpha));
    }

    // Draws a smooth outline hugging every silhouette flagged in `mask`. Pixels
    // within `thickness` of the edge get the solid outline color; beyond that a
    // quadratic falloff yields an anti-aliased glow out to `glow_radius`.
    //
    // The outline only ever appears next to a silhouette edge, so rather than
    // scanning a full kernel around every pixel (O(pixels * kernel)), we find the
    // boundary pixels once and scatter the glow outward from them only
    // (O(pixels + perimeter * kernel)). This keeps the pass cheap enough to run
    // on every hover change.
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

        // Per-pixel outline intensity; kept as the max (nearest edge) contribution.
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

        // Snapshot the current overlay state (selection + hover) and figure out
        // whether the expensive geometry pass has to run at all.
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

        // A selection change can accompany a scene mutation -- the UI piggybacks
        // show/hide toggles onto setSelection -- so it forces a full geometry
        // pass to stay correct. A hover change only ever tweaks the overlay, so
        // hover-only updates skip ray tracing and re-composite from the cached
        // buffers. Hover fires on every mouse-move across the viewport, so this
        // is the hot path the optimization targets.
        bool geometry_dirty = selection_changed || this->needsGeometryRefresh(scene);
        {
            std::lock_guard lock(_renderMutex);
            geometry_dirty = geometry_dirty || this->_baseColors.size() != pixel_count;
        }

        if (!geometry_dirty && !hover_changed)
            return;

        this->_rendering = true;
        this->_stopRequested = false;

        // Geometry pass: only when the camera/scene changed. Produces the base
        // colours and the primitive hit per pixel, both cached so overlay-only
        // updates (hover/selection) can skip ray tracing entirely.
        if (geometry_dirty)
        {
            const ViewportCameraData camera_data = build_viewport_camera_data(camera, resolution.x, resolution.y);
            std::vector<Color> frame_buffer(pixel_count, Color());
            std::vector<const IPrimitive *> id_buffer(pixel_count, nullptr);

            std::vector<std::thread> workers;
            std::atomic<int> next_tile = 0;

            const int tiles_x = (resolution.x + TILE_SIZE - 1) / TILE_SIZE;
            const int tiles_y = (resolution.y + TILE_SIZE - 1) / TILE_SIZE;
            const int total_tiles = tiles_x * tiles_y;
            const size_t thread_count = std::max<size_t>(1, std::thread::hardware_concurrency());

            auto render_tile = [&](int tile_x, int tile_y)
            {
                const int start_x = tile_x * TILE_SIZE;
                const int start_y = tile_y * TILE_SIZE;
                const int end_x = std::min(start_x + TILE_SIZE, resolution.x);
                const int end_y = std::min(start_y + TILE_SIZE, resolution.y);

                for (int y = start_y; y < end_y; ++y)
                {
                    for (int x = start_x; x < end_x; ++x)
                    {
                        if (this->_stopRequested)
                            return;
                        const Ray ray = viewport_primary_ray(camera_data, x, y);
                        ViewportHit hit = ray_hit(ray, scene);
                        const size_t idx = static_cast<size_t>(y) * static_cast<size_t>(resolution.x) + static_cast<size_t>(x);
                        frame_buffer[idx] = hit.color;
                        id_buffer[idx] = hit.hit ? hit.primitive : nullptr;
                    }
                }
            };

            auto worker = [&]
            {
                while (true)
                {
                    if (this->_stopRequested)
                        break;
                    const int tile = next_tile++;
                    if (tile >= total_tiles)
                        break;
                    render_tile(tile % tiles_x, tile / tiles_x);
                }
            };

            workers.reserve(thread_count);
            for (size_t i = 0; i < thread_count; ++i)
                workers.emplace_back(worker);
            for (auto &worker_thread : workers)
                worker_thread.join();

            if (this->_stopRequested)
            {
                this->_rendering = false;
                return;
            }

            std::lock_guard lock(_renderMutex);
            this->_baseColors = std::move(frame_buffer);
            this->_primitiveIds = std::move(id_buffer);
        }

        // Composite pass: rebuild the overlay (outlines + gizmos) over a fresh
        // copy of the cached base colours. Cheap enough to run every hover/select.
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
            this->_lastScene = &scene;
            this->_lastResolution = resolution;
            this->_lastCameraPosition = camera.getPosition();
            this->_lastCameraRotation = camera.getRotation();
            this->_lastCameraFov = camera.getFov();
            this->_lastSamplesPerPixel = camera.getSamplesPerPixel();
            this->_lastSelectionVersion = selection_version;
            this->_lastHoverVersion = hover_version;
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
