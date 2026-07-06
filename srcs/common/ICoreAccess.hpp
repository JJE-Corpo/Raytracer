//
// Created by jazema on 4/22/26.
//

#ifndef ICOREACCESS_HPP
#define ICOREACCESS_HPP

#include "ISceneRenderer.hpp"
#include "IPluginLoader.hpp"
#include "cluster/IClusterModule.hpp"

namespace rc
{
    class ICoreAccess
    {
        public:
            enum class CoreState
            {
                INITIALIZING,
                LOADING_SCENE,
                RENDERING,
                READY,
                EXITING,
            };
            virtual ~ICoreAccess() = default;

            virtual void clearScene() = 0;
            virtual void loadScene(const std::string &scene_path) = 0;
            virtual void saveScene(const std::string &scene_path) = 0;
            virtual IScene &loadNewScene(const std::string &scene_path) = 0;
            virtual ISceneRenderer *getRenderer() const = 0;
            virtual ISceneRenderer *getViewportRenderer() const = 0;
            virtual IPluginLoader *getPluginLoader() const = 0;

            virtual IClusterModule *getClusterModule() const = 0;

            virtual std::string getCurrentScenePath() = 0;

            // virtual void setFastMode(bool fastMode) = 0;

            virtual IScene *getScene() const = 0;

            virtual CoreState getState() const = 0;

            virtual void requestRender() = 0;
            virtual void stop() = 0;

            virtual void historyReset() = 0;
            virtual bool historyCapture() = 0;
            virtual bool historyUndo() = 0;
            virtual bool historyRedo() = 0;
    };
}

#endif
