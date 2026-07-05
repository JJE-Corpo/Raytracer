//
// Created by jazema on 4/21/26.
//

#ifndef CORE_HPP
#define CORE_HPP
#include <atomic>
#include <filesystem>
#include <memory>
#include <string>
#include <vector>

#include "PluginLoader.hpp"
#include "../common/IUserInterface.hpp"
#include "../common/ISceneRenderer.hpp"
#include "../common/ICoreAccess.hpp"
#include "scene/Scene.hpp"
#include "scene/SceneParser.hpp"
#include "utils/FileObserver.hpp"

namespace rc
{
    class Core : public ICoreAccess
    {
        private:
            PluginLoader _pluginLoader;
            SceneParser _sceneParser;
            FileObserver _configObserver;

            IUserInterface *_userInterface;
            IScene *_scene;

            ISceneRenderer *_defaultRenderer;
            ISceneRenderer *_clusteredRenderer;
            ISceneRenderer *_viewportRenderer;

            std::unique_ptr<class ClusterRenderer> _ownedClusterRenderer;

            IClusterModule *_clusterModule;

            std::atomic<bool> _renderRequested{};

            std::atomic<bool> _running;

            std::atomic<CoreState> _state;

            std::string _renderOutputPath = "render.png";

            // Undo/redo: full-scene JSON snapshots. _historyIndex points at the
            // snapshot matching the live scene; entries after it are the redo
            // tail. Camera state is diffed out (see historyCapture) and re-applied
            // on restore so camera moves never create undo steps.
            std::vector<nlohmann::json> _history;
            int _historyIndex = -1;
            static constexpr std::size_t HISTORY_LIMIT = 100;

            // Serialize the current scene, then restore a snapshot by swapping the
            // live scene for a freshly parsed one (preserving the live camera).
            nlohmann::json snapshotScene() const;
            void restoreSnapshot(const nlohmann::json &snapshot);
            // The camera-independent part of a snapshot, used to detect whether an
            // edit actually changed anything worth recording.
            static std::string historySignature(const nlohmann::json &snapshot);
        public:
            Core();
            ~Core() override;

            void loadUserInterface();
            void loadRenderers();

            void unloadUserInterface();
            void unloadRenderers();
            void unloadScene();

            void loadPlugins();

            std::string getCurrentScenePath() override;

            IClusterModule *getClusterModule() const override;

            // void setFastMode(bool fastMode) override;
            void requestRender() override;

            void renderFrame();

            void startRendering();
            void stop() override;

            void loadBlankScene();

            void setRenderOutput(const std::string &path);
            std::string resolveOutputPath(const std::string &path) const;

            CoreState getState() const override;

            // ICoreAccess implementation
            void clearScene() override;
            void loadScene(const std::string &scene_path) override;
            void saveScene(const std::string &scene_path) override;
            IScene &loadNewScene(const std::string &scene_path) override;
            ISceneRenderer *getRenderer() const override;
            ISceneRenderer *getViewportRenderer() const override;
            IPluginLoader *getPluginLoader() const override;
            IScene *getScene() const override;

            void historyReset() override;
            bool historyCapture() override;
            bool historyUndo() override;
            bool historyRedo() override;
    };
}

#endif
