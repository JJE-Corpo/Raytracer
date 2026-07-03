//
// Created by jazema on 5/4/26.
//

#ifndef RAYTRACER_VIEWPORTRENDERER_HPP
#define RAYTRACER_VIEWPORTRENDERER_HPP
#include "../../common/ISceneRenderer.hpp"
#include "../../common/ISelectionAwareRenderer.hpp"
#include <atomic>
#include <mutex>
#include <vector>

namespace rc
{
    class IPrimitive;
    class ILight;

    class ViewportRenderer : public ISceneRenderer, public ISelectionAwareRenderer
    {
        private:
            Render _render = {0, 0, std::vector<Color>()};
            mutable std::mutex _renderMutex;
            mutable std::mutex _cacheMutex;

            std::atomic<bool> _rendering = false;
            std::atomic<bool> _stopRequested = false;
            const IScene *_lastScene = nullptr;
            Vector2i _lastResolution = {-1, -1};
            Vector3f _lastCameraPosition = {0, 0, 0};
            Vector3f _lastCameraRotation = {0, 0, 0};
            double _lastCameraFov = 0.0;
            int _lastSamplesPerPixel = -1;
            std::vector<const ISceneObject *> _selection;
            size_t _selectionVersion = 0;
            size_t _lastSelectionVersion = 0;
            const ISceneObject *_hover = nullptr;
            size_t _hoverVersion = 0;
            size_t _lastHoverVersion = 0;
            bool needsRefresh(const IScene &scene) const;
        public:
            void renderScene(const IScene &scene) override;
            void setPixel(int x, int y, Color color) override;
            void setSelection(const std::vector<const ISceneObject *> &selection) override;
            void setHover(const ISceneObject *object) override;

            void stopRendering() override;

            bool isRendering() const override;
            int getCurrentSample() const override;

            std::string getRendererName() const override;
            PluginType getType() const override;
            Render getRender() const override;
    };
}

#endif
