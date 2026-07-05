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
        // Persistent badge in the viewport's bottom-left corner, drawn only while
        // movement mode is on, so the active camera controls are always visible
        // (the toggle toast is transient).
        void drawMovementIndicator(sf::RenderWindow &window);

        void updateSelectionFromClick(const sf::Vector2i &mouse);
        void updateHoverFromMouse(const sf::Vector2i &mouse);
        void clearHover();
        void syncSelectionToRenderer();
        void markViewportBvhDirty();
        void applyImport();
        void updateViewportCamera(sf::RenderWindow &window);

        // Keyboard shortcuts (Ctrl+S save, Ctrl+Z undo, Ctrl+Shift+Z redo).
        // handleShortcut consumes the matching key event and returns true so the
        // rest of handleEvent (routing, fly keys) is skipped for it.
        bool handleShortcut(const sf::Event &event);
        // Save the current scene (Ctrl+S and the Scene > Save menu item both
        // funnel here): overwrites the current file, or opens the save dialog
        // when the scene has no path yet.
        void triggerSave();
        void undoShortcut();
        void redoShortcut();
        // Re-sync the UI after undo/redo swapped the scene object out: the old
        // selection/hover pointers are dropped and the viewport rebuilt.
        void onSceneRestored();
        // True while a component (a focused text field, an open menu/pop-up) owns
        // the keyboard, so global shortcuts must not fire under it.
        bool isKeyboardCaptured();

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

        // Flip movement mode (bound to 'M'). Only while it is on do the right-drag
        // look and the ZQSD fly keys drive the camera; turning it off also drops
        // any held fly keys so the camera can't keep coasting.
        void toggleMovementMode();

        // True when the pointer currently "belongs to" the viewport, so the
        // camera fly/look controls may act. See the definition for the exact
        // rules (viewport mode, cursor over the render, nothing else capturing).
        bool isViewportCaptured(const sf::Vector2i &mouse);

        ICoreAccess *_coreAccess = nullptr;
        sf::Font *_font = nullptr;

        ViewMode _viewMode = ViewMode::VIEWPORT;
        bool _viewportBvhDirty = true;
        ISceneRenderer *_activeRenderer = nullptr;

        // Tracks which scene the undo/redo baseline belongs to. When the live
        // scene pointer changes for a reason other than our own undo/redo (a
        // load, an Open, a config reload), the history is reset to that scene.
        IScene *_lastHistoryScene = nullptr;

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
        // Camera controls (right-drag look + ZQSD fly) only act while this is on;
        // toggled with the 'M' key. Off by default so plain clicks/keys edit the
        // scene without nudging the camera.
        bool _movementMode = false;

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
