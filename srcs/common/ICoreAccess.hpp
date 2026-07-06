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

            // Undo/redo history. The history holds in-memory JSON snapshots of the
            // scene; camera state is excluded so navigating the camera is never an
            // undo step. historyReset() drops the stacks and takes the current
            // scene as the new baseline (call it whenever a scene is loaded).
            // historyCapture() folds the current scene into a new undo step, but
            // only when it actually differs from the last snapshot; it returns
            // true when a step was recorded. historyUndo()/historyRedo() swap the
            // live scene for the previous/next snapshot and return whether they
            // moved (false when there is nothing to undo/redo). A successful
            // undo/redo replaces the scene object, so callers must drop any cached
            // pointers into the old scene (selection, hover) afterwards.
            virtual void historyReset() = 0;
            virtual bool historyCapture() = 0;
            virtual bool historyUndo() = 0;
            virtual bool historyRedo() = 0;
    };
}

#endif
