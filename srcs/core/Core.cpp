//
// Created by jazema on 4/21/26.
//

#include "Core.hpp"

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <thread>

#include "../common/IUserInterface.hpp"
#include "cluster/ClusterModule.hpp"
#include "cluster/render/ClusterRenderer.hpp"
#include "cluster/server/ClusterServer.hpp"
#include "scene/builder/SceneBuilder.hpp"
#include "utils/RenderExporter.hpp"
#include "scene/SceneRegister.hpp"
#include "scene/factory/PrimitiveFactory.hpp"

namespace rc
{
    Core::Core() : _userInterface(nullptr), _scene(nullptr), _defaultRenderer(nullptr), _clusteredRenderer(nullptr), _viewportRenderer(nullptr), _running(false), _state(CoreState::INITIALIZING)
    {
        this->_clusterModule = new ClusterModule();
        this->_ownedClusterRenderer = std::make_unique<ClusterRenderer>();
        this->_clusteredRenderer = this->_ownedClusterRenderer.get();
    }

    Core::~Core()
    {
        unloadRenderers();
        unloadScene();
        unloadUserInterface();
        delete (this->_clusterModule);
    }

    void Core::loadUserInterface()
    {
        std::vector<PluginLoader::PluginHandle> interfaces;

        if (this->_userInterface)
            unloadUserInterface();
        interfaces = this->_pluginLoader.getPlugins(PluginType::USER_INTERFACE);
        if (interfaces.empty())
        {
            std::cout << "Could not find any user interface.. Skipping" << std::endl;
            return;
        }
        this->_userInterface = dynamic_cast<IUserInterface *>(interfaces[0].instance);
        if (this->_userInterface == nullptr)
        {
            std::cout << "Could not find any user interface.. Skipping" << std::endl;
            return;
        }
        this->_userInterface->create(*this);
    }

    void Core::loadScene(const std::string &scene_path)
    {
        if (this->_state != CoreState::READY && this->_state != CoreState::INITIALIZING)
            return;
        this->_state = CoreState::LOADING_SCENE;
        this->_configObserver = FileObserver(scene_path);
        if (this->_scene)
            unloadScene();
        this->_scene = this->_sceneParser.parseScene(scene_path);
        this->_state = CoreState::READY;
    }

    void Core::loadBlankScene()
    {
        this->_scene = new Scene(new Camera({1280, 720}, {0, 0, 5}, {0, 0, 0}, 80));
    }

    void Core::setRenderOutput(const std::string &path)
    {
        if (!path.empty())
            this->_renderOutputPath = path;
    }

    std::string Core::resolveOutputPath(const std::string &path) const
    {
        std::filesystem::path output(path);

        if (output.is_relative())
        {
            const char *originalCwd = std::getenv("RAYTRACER_CWD");
            if (originalCwd != nullptr && originalCwd[0] != '\0')
                output = std::filesystem::path(originalCwd) / output;
        }
        return (output.string());
    }

    void Core::loadRenderers()
    {
        for (auto &plugin : this->_pluginLoader.getPlugins(PluginType::RENDERER))
        {
            auto candidate = dynamic_cast<ISceneRenderer *>(plugin.instance);
            if (!candidate)
                continue;
            if (candidate->getRendererName().find("Default") != std::string::npos)
                this->_defaultRenderer = candidate;
            else if (candidate->getRendererName().find("Viewport") != std::string::npos)
                this->_viewportRenderer = candidate;
        }
        if (this->_clusteredRenderer == nullptr && this->_ownedClusterRenderer)
            this->_clusteredRenderer = this->_ownedClusterRenderer.get();
        if (this->_defaultRenderer == nullptr)
            throw std::runtime_error("Default renderer could not be loaded");
    }

    void Core::unloadUserInterface()
    {
        if (!this->_userInterface)
            return;
        this->_userInterface->destroy();
    }

    void Core::unloadScene()
    {
        if (!this->_scene)
            return;
        delete (this->_scene);
        this->_scene = nullptr;
    }

    void Core::unloadRenderers()
    {
        this->_defaultRenderer = nullptr;
        this->_clusteredRenderer = this->_ownedClusterRenderer.get();
        this->_viewportRenderer = nullptr;
    }

    void Core::loadPlugins()
    {
        for (const auto &entry : std::filesystem::directory_iterator("./plugins"))
        {
            if (entry.path().extension() == ".so")
                this->_pluginLoader.load(entry.path());
        }
        auto renderers = this->_pluginLoader.getPlugins(PluginType::RENDERER);
        std::cout << "Loaded " << renderers.size() << " renderer plugin(s)" << std::endl;
        for (const auto &p : renderers)
        {
            auto r = dynamic_cast<ISceneRenderer *>(p.instance);
            if (r)
                std::cout << " - " << r->getRendererName() << std::endl;
            else
                std::cout << " - (unknown renderer instance)" << std::endl;
        }
    }

    IClusterModule *Core::getClusterModule() const
    {
        return (this->_clusterModule);
    }

    void Core::requestRender()
    {
        this->_renderRequested.exchange(true);
    }
    
    std::string Core::getCurrentScenePath()
    {
        return (this->_configObserver.getFilePath());
    }
    

    void Core::renderFrame()
    {
        if (this->_state != CoreState::READY)
            return;
        this->_state = CoreState::RENDERING;
        this->_scene->buildBvh();
        if (this->getClusterModule()->getClusterMode() == ClusterMode::SERVER)
        {
            auto *clusterRenderer = dynamic_cast<ClusterRenderer *>(this->_clusteredRenderer);
            auto *clusterServer = dynamic_cast<ClusterServer *>(this->getClusterModule()->getClusterServer());
            if (clusterRenderer)
                clusterRenderer->setClusterServer(clusterServer);
        }
        this->getRenderer()->renderScene(*this->_scene);
        std::cout << "Finished rendering scene! Saving to file.." << std::endl;
        std::string outputPath = this->resolveOutputPath(this->_renderOutputPath);
        RenderExporter::saveToFile(this->getRenderer()->getRender(), outputPath);
        std::cout << "Render has been saved to file '" << outputPath << "'" << std::endl;
        this->_state = CoreState::READY;
    }

    void Core::startRendering()
    {
        if (this->_state != CoreState::INITIALIZING && this->_state != CoreState::READY)
            return;
        this->_renderRequested = false;
        this->_state = CoreState::READY;
        if (this->_userInterface == nullptr)
        {
            this->renderFrame();
            return;
        }
        this->_scene->buildBvh();
        this->_running = true;
        while (this->_running)
        {
            bool reloadScene = this->_configObserver.pollChanges();
            if (reloadScene && !this->_suppressWatcherReload)
                this->loadScene(this->_configObserver.getFilePath());
            if (this->getClusterModule()->getClusterMode() == ClusterMode::CLIENT)
            {
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
                continue;
            }
            if (this->_renderRequested.exchange(false))
            {
                std::cout << "Rendering full frame..." << std::endl;
                this->renderFrame();
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
        this->_state = CoreState::EXITING;
    }

    void Core::stop()
    {
        if (this->getRenderer() && this->getRenderer()->isRendering())
            this->getRenderer()->stopRendering();
        while (this->getRenderer()->isRendering())
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        this->_running = false;
        this->_state = CoreState::EXITING;
    }

    void Core::clearScene()
    {
        if (!this->_scene)
            return;
        this->_scene->clear();
    }

    void Core::saveScene(const std::string &scene_path)
    {
        SceneRegister registerer;

        const bool watchingTarget = (scene_path == this->_configObserver.getFilePath());

        if (watchingTarget)
            this->_suppressWatcherReload = true;

        registerer.saveScene(scene_path, this->_scene);

        if (watchingTarget)
        {
            this->_configObserver.reset();
            this->_suppressWatcherReload = false;
        }
    }

    IScene &Core::loadNewScene(const std::string &scene_path)
    {
        return *this->_sceneParser.parseScene(scene_path);
    }

    IScene *Core::getScene() const
    {
        return (this->_scene);
    }

    nlohmann::json Core::snapshotScene() const
    {
        return (SceneRegister::serializeScene(this->_scene));
    }

    std::string Core::historySignature(const nlohmann::json &snapshot)
    {
        nlohmann::json cameraless = snapshot;

        cameraless.erase("camera");
        return (cameraless.dump());
    }

    void Core::restoreSnapshot(const nlohmann::json &snapshot)
    {
        ICamera &current = this->_scene->getCamera();
        const Vector3f position = current.getPosition();
        const Vector3f rotation = current.getRotation();
        const double fov = current.getFov();
        const Vector2i resolution = current.getResolution();
        const int samples = current.getSamplesPerPixel();

        IScene *restored = this->_sceneParser.parseScene(snapshot);

        delete (this->_scene);
        this->_scene = restored;

        ICamera &camera = this->_scene->getCamera();
        camera.setPosition(position);
        camera.setRotation(rotation);
        camera.setFov(fov);
        camera.setResolution(resolution);
        camera.setSamplesPerPixel(samples);
    }

    void Core::historyReset()
    {
        this->_history.clear();
        this->_historyIndex = -1;
        if (!this->_scene)
            return;
        this->_history.push_back(this->snapshotScene());
        this->_historyIndex = 0;
    }

    bool Core::historyCapture()
    {
        if (!this->_scene)
            return (false);
        if (this->_historyIndex < 0)
        {
            this->historyReset();
            return (true);
        }

        nlohmann::json snapshot = this->snapshotScene();
        if (historySignature(snapshot) == historySignature(this->_history[this->_historyIndex]))
            return (false);

        if (this->_historyIndex + 1 < static_cast<int>(this->_history.size()))
            this->_history.erase(this->_history.begin() + this->_historyIndex + 1, this->_history.end());
        this->_history.push_back(std::move(snapshot));

        if (this->_history.size() > HISTORY_LIMIT)
            this->_history.erase(this->_history.begin(), this->_history.begin() + (this->_history.size() - HISTORY_LIMIT));
        this->_historyIndex = static_cast<int>(this->_history.size()) - 1;
        return (true);
    }

    bool Core::historyUndo()
    {
        if (!this->_scene || this->_historyIndex <= 0)
            return (false);
        this->restoreSnapshot(this->_history[this->_historyIndex - 1]);
        this->_historyIndex -= 1;
        return (true);
    }

    bool Core::historyRedo()
    {
        if (!this->_scene || this->_historyIndex < 0
            || this->_historyIndex + 1 >= static_cast<int>(this->_history.size()))
            return (false);
        this->restoreSnapshot(this->_history[this->_historyIndex + 1]);
        this->_historyIndex += 1;
        return (true);
    }

    ISceneRenderer *Core::getRenderer() const
    {
        if (this->_clusterModule && this->_clusterModule->getClusterMode() == ClusterMode::SERVER)
            return (this->_clusteredRenderer ? this->_clusteredRenderer : this->_defaultRenderer);
        return (this->_defaultRenderer);
    }

    ICoreAccess::CoreState Core::getState() const
    {
        return (this->_state);
    }

    ISceneRenderer *Core::getViewportRenderer() const
    {
        return (this->_viewportRenderer);
    }

    IPluginLoader *Core::getPluginLoader() const
    {
        return const_cast<PluginLoader *>(&this->_pluginLoader);
    }
}
