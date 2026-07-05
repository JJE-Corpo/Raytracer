//
// Created by jazema on 5/16/26.
//
// The "normal" editing screen: menu bar, resizable sidebar
// (hierarchy/camera/object/material sections) and the viewport/render panel.
// This is the counterpart to ClusterClientScreen - both are driven the same
// way by UserInterface (setFont/handleEvent/prepareFrame/tick/shutdown).
//

#ifndef DEFAULTSCREEN_HPP
#define DEFAULTSCREEN_HPP

#include <functional>
#include <string>

#include <SFML/System/Clock.hpp>
#include <SFML/Window/Keyboard.hpp>

#include "AScreen.hpp"
#include "../components/menu/MenuBar.hpp"
#include "../components/menu/ContextMenu.hpp"
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

    class DefaultScreen : public AScreen
    {
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
        void drawRenderer(sf::RenderWindow &window, ISceneRenderer *renderer);

        void updateSelectionFromClick(const sf::Vector2i &mouse);
        void updateHoverFromMouse(const sf::Vector2i &mouse);
        void clearHover();
        void syncSelectionToRenderer();
        void markViewportBvhDirty();
        void applyImport();
        void updateViewportCamera(sf::RenderWindow &window);

        // Object under the cursor in the viewport (light gizmo first, then a ray
        // cast), or nullptr. Shared by the right-click context menu.
        const ISceneObject *pickViewportObject(const sf::Vector2i &mouse);
        // Fill and open the object context menu at the (window-space) cursor for
        // the given object. Both the hierarchy right-click and the viewport
        // right-click funnel through here.
        void openContextMenu(const ISceneObject *object, const sf::Vector2i &mouse);
        // Toggle the hidden flag of the current selection (context-menu action).
        void hideSelection();
        // Create a new top-level group and move the current (multi-)selection
        // into it, then select the group (context-menu action).
        void groupSelection();

        // Latch the fly-camera key state from window key events. Done at event
        // time (before the frame's render) so a slow render frame can never make
        // the once-per-frame poll miss a press or release.
        void trackFlyKeys(const sf::Event &event, const sf::Vector2i &mouse);
        void setFlyKey(sf::Keyboard::Key key, bool pressed);
        void resetFlyKeys();

        // True when the pointer currently "belongs to" the viewport, so the
        // camera fly/look controls may act. See the definition for the exact
        // rules (viewport mode, cursor over the render, nothing else capturing).
        bool isViewportCaptured(const sf::Vector2i &mouse);

        ICoreAccess *_coreAccess = nullptr;
        sf::Font *_font = nullptr;

        ViewMode _viewMode = ViewMode::VIEWPORT;
        bool _viewportBvhDirty = true;
        ISceneRenderer *_activeRenderer = nullptr;

        MenuBar _menuBar;

        // Right-click object context menu (hierarchy rows and viewport objects).
        ContextMenu _contextMenu;

        // Resizable left sidebar.
        float _sidebarWidth = 260.0f;
        ResizeHandle _sidebarResize;

        HierarchyPanel _hierarchyPanel;
        ObjectPanel _objectPanel;
        CameraPanel _cameraPanel;
        MaterialPanel _materialPanel;

        // Resizable/scrollable stack wrapping the panels above.
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

        bool _loadWindowPending = false;
        std::function<void()> _loadWindowOnClose;

        ToastManager _toastManager;

        // viewport
        sf::Vector2i _lastMouse;
        bool _rightMouseHeld = false;
        // Right-button click-vs-drag tracking: a drag rotates the camera, a
        // click (no drag past the threshold) opens the object context menu.
        sf::Vector2i _rightPressMouse;
        bool _rightPressInViewport = false;
        bool _rightDragged = false;
        // Fly-camera speed in world units per second (integrated with real frame
        // time so motion is smooth and independent of frame rate / render cost).
        float _cameraSpeed = 6.0f;
        sf::Clock _frameClock;
        // Held fly keys, latched from key events (see trackFlyKeys). Movement is
        // driven by these rather than by polling, so a press captured over the
        // viewport keeps moving the camera while held even if the cursor drifts.
        bool _keyForward = false;
        bool _keyBack = false;
        bool _keyLeft = false;
        bool _keyRight = false;
        bool _keyUp = false;
        bool _keyDown = false;

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
    };
}

#endif
