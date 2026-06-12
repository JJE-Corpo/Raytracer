//
// Created by jazema on 4/21/26.
//

#include "DefaultRenderer.hpp"

#include <algorithm>
#include <atomic>
#include <iostream>
#include <random>
#include <thread>
#include <vector>

#include "../../common/render/RenderKernel.hpp"
#include "../../common/scene/IScene.hpp"

namespace rc
{
    constexpr int TILE_SIZE = 64;

    void DefaultRenderer::renderScene(const IScene &scene)
    {
        const ICamera &camera = scene.getCamera();
        const Vector2i res = camera.getResolution();

        this->_rendering = true;
        this->_stopRequested = false;

        {
            std::lock_guard<std::mutex> lock(_renderMutex);
            this->_render.size_x = res.x;
            this->_render.size_y = res.y;
            this->_render.pixels.resize(res.x * res.y);
        }

        this->_currentSample = 0;

        std::vector accum(res.x * res.y, ColorF{0,0,0});

        int tiles_x = (res.x + TILE_SIZE - 1) / TILE_SIZE;
        int tiles_y = (res.y + TILE_SIZE - 1) / TILE_SIZE;
        int total_tiles = tiles_x * tiles_y;

        size_t thread_count = std::thread::hardware_concurrency();
        std::vector<std::thread> threads;

        int spp = camera.getSamplesPerPixel();

        std::vector<int> tile_order(total_tiles);
        std::iota(tile_order.begin(), tile_order.end(), 0);
        std::shuffle(tile_order.begin(), tile_order.end(), std::mt19937{std::random_device{}()});

        for (int sample = 0; sample < spp; sample++)
        {
            if (this->_stopRequested)
                break;
            this->_currentSample = sample;
            std::atomic<int> next_tile = 0;

            auto worker = [&]()
            {
                while (true)
                {
                    if (this->_stopRequested)
                        break;
                    int idx = next_tile++;
                    if (idx >= total_tiles) break;

                    int tile = tile_order[idx];

                    int tx = tile % tiles_x;
                    int ty = tile / tiles_x;

                    int start_x = tx * TILE_SIZE;
                    int start_y = ty * TILE_SIZE;
                    int end_x = std::min(start_x + TILE_SIZE, res.x);
                    int end_y = std::min(start_y + TILE_SIZE, res.y);

                    for (int y = start_y; y < end_y; y++)
                    {
                        for (int x = start_x; x < end_x; x++)
                        {
                            if (this->_stopRequested)
                                return;
                            Ray r = camera.generateRay(x, y);
                            ColorF c = render_kernel::trace_ray(r, scene, 10);

                            int i = y * res.x + x;
                            accum[i] = accum[i] + c;
                        }
                    }
                }
            };

            threads.clear();
            for (size_t i = 0; i < thread_count; i++)
                threads.emplace_back(worker);

            for (auto &t : threads)
                t.join();

            if (this->_stopRequested)
                break;
            float scale = 1.0f / (sample + 1);

            {
                std::lock_guard<std::mutex> lock(_renderMutex);
                for (int i = 0; i < res.x * res.y; i++)
                {
                    ColorF c = accum[i] * scale;

                    this->_render.pixels[i] = c.toColor();
                }
            }
        }

        this->_currentSample = -1;

        this->_rendering = false;
    }

    void DefaultRenderer::stopRendering()
    {
        this->_stopRequested = true;
    }

    bool DefaultRenderer::isRendering() const
    {
        return (this->_rendering);
    }

    int DefaultRenderer::getCurrentSample() const
    {
        return (this->_currentSample);
    }

    void DefaultRenderer::setPixel(const int x, const int y, const Color color)
    {
        std::lock_guard<std::mutex> lock(_renderMutex);
        if (x >= 0 && y >= 0 && x < _render.size_x && y < _render.size_y)
            this->_render.pixels[y * this->_render.size_x + x] = color;
    }

    Render DefaultRenderer::getRender() const
    {
        std::lock_guard<std::mutex> lock(_renderMutex);
        return (this->_render);
    }

    std::string DefaultRenderer::getRendererName() const
    {
        return ("Default");
    }

    PluginType DefaultRenderer::getType() const
    {
        return (PluginType::RENDERER);
    }

    extern "C" IPlugin *create_plugin()
    {
        return new DefaultRenderer();
    }

    extern "C" void destroy_plugin(IPlugin* plugin)
    {
        delete plugin;
    }
}
