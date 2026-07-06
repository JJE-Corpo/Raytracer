//
// Created by jazema on 5/16/26.
//

#include "ClusterRenderer.hpp"

#include <thread>

#include "../../../common/render/RenderKernel.hpp"
#include "../server/ClusterServer.hpp"

namespace rc
{
    void ClusterRenderer::setClusterServer(ClusterServer *server)
    {
        this->_server = server;
    }

    void ClusterRenderer::applyTileSample(const ClusterRenderCoordinator::TileJob &job, const std::vector<ColorF> &pixels)
    {
        const int tile_w = job.end_x - job.start_x;
        const int tile_h = job.end_y - job.start_y;
        const float scale = 1.0f / static_cast<float>(job.sample + 1);

        std::lock_guard lock(this->_renderMutex);
        if (this->_width <= 0 || this->_height <= 0)
            return;
        if (this->_accum.size() != static_cast<size_t>(this->_width * this->_height))
            return;
        if (static_cast<size_t>(tile_w * tile_h) != pixels.size())
            return;

        for (int y = 0; y < tile_h; ++y)
        {
            for (int x = 0; x < tile_w; ++x)
            {
                const int px = job.start_x + x;
                const int py = job.start_y + y;
                if (px < 0 || py < 0 || px >= this->_width || py >= this->_height)
                    continue;
                const size_t tile_idx = static_cast<size_t>(y) * static_cast<size_t>(tile_w) + static_cast<size_t>(x);
                const size_t idx = static_cast<size_t>(py) * static_cast<size_t>(this->_width) + static_cast<size_t>(px);
                this->_accum[idx] = this->_accum[idx] + pixels[tile_idx];
                ColorF c = this->_accum[idx] * scale;
                this->_render.pixels[idx] = c.toColor();
            }
        }
    }

    void ClusterRenderer::renderScene(const IScene &scene)
    {
        const ICamera &camera = scene.getCamera();
        const Vector2i res = camera.getResolution();
        const int spp = camera.getSamplesPerPixel();

        if (res.x <= 0 || res.y <= 0)
            return;

        this->_rendering = true;
        this->_stopRequested = false;

        {
            std::lock_guard lock(this->_renderMutex);
            this->_width = res.x;
            this->_height = res.y;
            this->_render.size_x = res.x;
            this->_render.size_y = res.y;
            this->_render.pixels.assign(static_cast<size_t>(res.x * res.y), Color());
            this->_accum.assign(static_cast<size_t>(res.x * res.y), ColorF{0.0f, 0.0f, 0.0f});
        }

        ClusterServer *server = this->_server;
        if (server)
        {
            server->setRenderCoordinator(&this->_coordinator, this);
            server->broadcastScene();
            server->broadcastServerState(ServerRenderState::RENDERING);
        }

        for (int sample = 0; sample < spp; ++sample)
        {
            if (this->_stopRequested)
                break;
            this->_currentSample = sample;
            this->_coordinator.beginSample(static_cast<uint32_t>(sample), res.x, res.y, this->_tileSize);

            while (!this->_stopRequested)
            {
                ClusterRenderCoordinator::TileJob job;
                if (this->_coordinator.popJob(job))
                {
                    std::vector<ColorF> pixels;
                    render_kernel::render_tile_sample(scene, camera, job.start_x, job.start_y, job.end_x, job.end_y, pixels);
                    if (this->_coordinator.markComplete(job))
                        this->applyTileSample(job, pixels);
                    continue;
                }

                this->_coordinator.requeueTimedOut(this->_tileTimeout);
                if (this->_coordinator.waitForSampleCompletion(std::chrono::milliseconds(10)))
                    break;
            }
        }

        if (server)
            server->clearRenderCoordinator();

        this->_currentSample = -1;
        this->_rendering = false;
        if (server)
            server->broadcastServerState(ServerRenderState::IDLING);
    }

    void ClusterRenderer::stopRendering()
    {
        this->_stopRequested = true;
        this->_coordinator.cancel();
        if (this->_server)
            this->_server->broadcastCancelRender();
    }

    bool ClusterRenderer::isRendering() const
    {
        return this->_rendering;
    }

    int ClusterRenderer::getCurrentSample() const
    {
        return (this->_currentSample);
    }

    void ClusterRenderer::setPixel(int x, int y, Color color)
    {
        std::lock_guard lock(this->_renderMutex);
        if (x >= 0 && y >= 0 && x < this->_render.size_x && y < this->_render.size_y)
            this->_render.pixels[y * this->_render.size_x + x] = color;
    }

    Render ClusterRenderer::getRender() const
    {
        std::lock_guard lock(this->_renderMutex);
        return (this->_render);
    }

    std::string ClusterRenderer::getRendererName() const
    {
        return ("Cluster");
    }

    PluginType ClusterRenderer::getType() const
    {
        return (PluginType::RENDERER);
    }
}
