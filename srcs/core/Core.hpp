//
// Created by jazema on 4/21/26.
//

#ifndef CORE_HPP
#define CORE_HPP
#include <atomic>
#include <filesystem>
#include <memory>
#include <string>

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
    };
}

#endif
