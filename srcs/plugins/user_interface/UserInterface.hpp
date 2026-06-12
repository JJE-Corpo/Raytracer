//
// Created by jazema on 4/21/26.
//

#ifndef USERINTERFACE_HPP
#define USERINTERFACE_HPP

#include <SFML/Graphics.hpp>
#include <thread>
#include <mutex>
#include <atomic>
#include <condition_variable>

#include "Component.hpp"
#include "windows/JoinClusterWindow.hpp"
#include "toast/ToastManager.hpp"
#include "../../common/IPlugin.hpp"
#include "../../common/ISceneRenderer.hpp"
#include "../../common/IUserInterface.hpp"
#include "../../common/ICoreAccess.hpp"
#include "components/ColorPicker.hpp"
#include "components/Separator.hpp"
#include "components/menu/MenuBar.hpp"
#include "layouts/ClusterClientLayout.hpp"
#include "layouts/DefaultLayout.hpp"
#include "panels/CameraPanel.hpp"
#include "panels/HierarchyPanel.hpp"
#include "panels/MaterialPanel.hpp"
#include "panels/ObjectPanel.hpp"
#include "panels/RendererPanel.hpp"
#include "windows/ExploratorWindow.hpp"
#include "windows/LoadWindow.hpp"

constexpr float padding    = 20.f;
constexpr float itemHeight = 40.f;

namespace rc
{
    class UserInterface : public IUserInterface
    {
        enum class ViewMode
        {
            VIEWPORT,
            RENDERING
        };

        private:
            sf::RenderWindow _window;
            sf::Font _font;

            std::thread _uiThread;
            std::mutex _windowMutex;
            std::atomic<bool> _running;

            ICoreAccess *_coreAccess;

            ViewMode _viewMode = ViewMode::VIEWPORT;
            bool _viewportBvhDirty = true;

            DefaultLayout _defaultLayout;
            ClusterClientLayout _clusterClientLayout;

            MenuBar _menuBar;

            HierarchyPanel _hierarchyPanel;
            Separator _sepSelection;
            ObjectPanel _objectPanel;
            CameraPanel _cameraPanel;
            Separator _sepMaterial;
            MaterialPanel _materialPanel;

            RendererPanel _rendererPanel;

            // Windows
            // Elements de l'UI
            bool _isNewScene = false;
            JoinClusterWindow _joinClusterWindow;
            LoadWindow _loadWindow;
            ExploratorWindow _exploratorWindow;

            std::string _exploratorResult;
            bool _exploratorJustClosed = false;
            std::function<void()> _exploratorOnClose;

            ToastManager _toastManager;

            // Curseurs
            sf::Cursor _cursorArrow;
            sf::Cursor _cursorHand;
            sf::Cursor _cursorText;
            sf::Cursor _cursorNotAllowed;
            sf::Cursor _cursorViewport;
            //

            // viewport
            sf::Vector2i _lastMouse;
            bool _rightMouseHeld = false;
            float _cameraSpeed = 5.0f;
            //

            void buildUI();
            void updateUI();

            void setupLayout(Layout &layout);

            void drawUI();
            void drawRenderer(ISceneRenderer *renderer);

            void setCursor(CursorType cursorType);

            void updateSelectionFromClick(const sf::Vector2i &mouse);
            void syncSelectionToRenderer();
            void markViewportBvhDirty();

            void updateViewportCamera();

            void handleWindowEvent(const sf::Event &event);
            void eventLoop();

            void exit();
            bool shouldExit() const;
        public:
            UserInterface();
            ~UserInterface() override;

            void create(ICoreAccess &core_access) override;
            void destroy() override;

            // IPlugin abstract
            PluginType getType() const override;
    };

}

#endif
