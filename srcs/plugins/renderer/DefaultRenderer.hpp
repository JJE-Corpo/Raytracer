//
// Created by jazema on 4/21/26.
//

#ifndef DEFAULTRENDERER_HPP
#define DEFAULTRENDERER_HPP

#include <atomic>
#include <mutex>

#include "../../common/ISceneRenderer.hpp"

namespace rc
{
    struct Color;

    class DefaultRenderer : public ISceneRenderer
    {
        private:
            Render _render = {0, 0, std::vector<Color>()};
            mutable std::mutex _renderMutex;

            std::atomic<bool> _rendering = false;
            std::atomic<bool> _stopRequested = false;

            std::atomic<int> _currentSample = -1;

            // static ColorF compute_direct(const Intersection &hit, const IScene &scene);
            // static ColorF compute_ambient(const Intersection &hit, const IScene &scene);
            // static ColorF compute_indirect(const Intersection &hit, const IScene &scene, int depth);

            // static Color ray_color(const Ray &r, const IScene &scene, int depth = 0);
        public:
            void renderScene(const IScene &scene) override;
            void setPixel(int x, int y, Color color) override;

            void stopRendering() override;

            bool isRendering() const override;
            int getCurrentSample() const override;

            Render getRender() const override;
            std::string getRendererName() const override;
            PluginType getType() const override;
    };
}

#endif
