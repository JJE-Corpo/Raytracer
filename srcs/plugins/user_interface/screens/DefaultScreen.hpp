//
// Created by jazema on 5/16/26.
//
// The "normal" editing screen: menu bar, resizable/collapsible sidebar
// (hierarchy/camera/object/material sections) and the viewport/render panel.
// This is the counterpart to ClusterClientScreen - both are driven the same
// way by UserInterface (setFont/handleEvent/prepareFrame/tick/shutdown).
//

#ifndef DEFAULTSCREEN_HPP
#define DEFAULTSCREEN_HPP

#include <functional>
#include <string>

#include "Screen.hpp"
#include "../components/menu/MenuBar.hpp"
#include "../components/ResizeHandle.hpp"
#include "../panels/CameraPanel.hpp"
#include "../panels/HierarchyPanel.hpp"
#include "../panels/MaterialPanel.hpp"
#include "../panels/ObjectPanel.hpp"
#include "../panels/RendererPanel.hpp"
#include "../panels/SidebarStack.hpp"
#include "../toast/ToastManager.hpp"
#include "../windows/ExploratorWindow.hpp"
#include "../windows/JoinClusterWindow.hpp"
#include "../windows/LoadWindow.hpp"
#include "../../../common/ICoreAccess.hpp"
#include "../../../common/ISceneRenderer.hpp"

namespace rc
{
    class IClusterClient;

    class DefaultScreen : public Screen
    {
        public:
            // UserInterface wires this to hand a just-joined cluster client over
            // to ClusterClientScreen (the two screens are siblings, this is the
            // only point where DefaultScreen needs to reach across).
            std::function<void(IClusterClient *)> onClusterJoined = [](IClusterClient *) {};

            void setCoreAccess(ICoreAccess *coreAccess);
            void buildUI();

            void setFont(sf::Font &font) override;
            void handleEvent(sf::RenderWindow &window, sf::Event &event) override;
            void prepareFrame() override;
            void shutdown() override;

        protected:
            void update(sf::RenderWindow &window) override;
            void draw(sf::RenderWindow &window) override;

        private:
            enum class ViewMode
            {
                VIEWPORT,
                RENDERING
            };

            void setupSidebarSection(SidebarStack::Slot slot, const std::string &id, const std::string &title,
                Component *content, std::function<void(float, float, float)> layoutContent,
                std::function<float()> contentHeight);

            void layoutSidebarResize(const sf::RenderWindow &window);
            void refreshSidebarVisibility();
            void setCursor(sf::RenderWindow &window, CursorType cursorType);
            void drawRenderer(sf::RenderWindow &window, ISceneRenderer *renderer);

            void updateSelectionFromClick(const sf::Vector2i &mouse);
            void syncSelectionToRenderer();
            void markViewportBvhDirty();
            void updateViewportCamera(sf::RenderWindow &window);

            ICoreAccess *_coreAccess = nullptr;
            sf::Font *_font = nullptr;

            ViewMode _viewMode = ViewMode::VIEWPORT;
            bool _viewportBvhDirty = true;
            ISceneRenderer *_activeRenderer = nullptr;

            MenuBar _menuBar;

            // Resizable left sidebar.
            float _sidebarWidth = 260.0f;
            ResizeHandle _sidebarResize;

            HierarchyPanel _hierarchyPanel;
            ObjectPanel _objectPanel;
            CameraPanel _cameraPanel;
            MaterialPanel _materialPanel;

            // Collapsible/resizable/scrollable stack wrapping the panels above.
            SidebarStack _sidebar;

            RendererPanel _rendererPanel;

            // Windows
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
            sf::Cursor _cursorResize;
            sf::Cursor _cursorResizeV;

            // viewport
            sf::Vector2i _lastMouse;
            bool _rightMouseHeld = false;
            float _cameraSpeed = 5.0f;
    };
}

#endif
