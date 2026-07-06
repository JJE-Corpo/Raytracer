//
// Created by jazema on 5/16/26.
//

#ifndef CLUSTERRENDERER_HPP
#define CLUSTERRENDERER_HPP

#include <atomic>
#include <chrono>
#include <mutex>
#include <string>
#include <vector>

#include "../../../common/ISceneRenderer.hpp"
#include "ClusterRenderCoordinator.hpp"
#include "IClusterTileSink.hpp"

namespace rc
{
    class ClusterServer;

    class ClusterRenderer : public ISceneRenderer, public IClusterTileSink
    {
        public:
            void renderScene(const IScene &scene) override;
            void setPixel(int x, int y, Color color) override;

            void stopRendering() override;

            bool isRendering() const override;
            int getCurrentSample() const override;

            Render getRender() const override;
            std::string getRendererName() const override;
            PluginType getType() const override;

            void setClusterServer(ClusterServer *server);

            void applyTileSample(const ClusterRenderCoordinator::TileJob &job, const std::vector<ColorF> &pixels) override;
        private:
            Render _render = {0, 0, std::vector<Color>()};
            mutable std::mutex _renderMutex;
            std::vector<ColorF> _accum;

            std::atomic<bool> _rendering = false;
            std::atomic<bool> _stopRequested = false;
            std::atomic<int> _currentSample = -1;

            ClusterServer *_server = nullptr;
            ClusterRenderCoordinator _coordinator;
            int _width = 0;
            int _height = 0;
            int _tileSize = 32;
            std::chrono::milliseconds _tileTimeout{5000};
    };
}

#endif
