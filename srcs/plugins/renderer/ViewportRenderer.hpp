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
            // Cached geometry pass: base colors and the primitive hit at each pixel.
            // Reused across overlay-only (selection/hover) changes so those never
            // trigger a full re-raytrace of the scene.
            std::vector<Color> _baseColors;
            std::vector<const IPrimitive *> _primitiveIds;
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
            // Set when the last geometry pass was a coarse (draft) render done
            // while the camera was moving; forces one full-resolution refine pass
            // on the next frame once the camera settles.
            bool _pendingRefine = false;
            // Set after a crisp full-resolution pass; schedules one edge-adaptive
            // anti-aliasing pass on the next idle frame to smooth silhouettes.
            bool _pendingAA = false;
            // Set by markSceneDirty() when the scene contents change without a
            // camera move; forces the next geometry pass so edits made in the
            // object panel (transform, material, light) show up immediately.
            bool _sceneDirty = false;
            std::vector<const ISceneObject *> _selection;
            size_t _selectionVersion = 0;
            size_t _lastSelectionVersion = 0;
            const ISceneObject *_hover = nullptr;
            size_t _hoverVersion = 0;
            size_t _lastHoverVersion = 0;
            bool needsGeometryRefresh(const IScene &scene) const;
        public:
            void renderScene(const IScene &scene) override;
            void setPixel(int x, int y, Color color) override;
            void markSceneDirty() override;
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
