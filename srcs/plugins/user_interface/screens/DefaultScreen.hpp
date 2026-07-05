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
#include "../../../common/scene/IEditablePrimitive.hpp"

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

        // --- Vertex edit mode ------------------------------------------------
        // True while any panel/pop-up/text field owns the input, so viewport
        // shortcuts (Tab/Escape) must not fire.
        bool anyUiCapturing();
        // The single-selected object cast to IEditablePrimitive, or nullptr.
        IEditablePrimitive *editableFromSelection() const;
        void toggleEditMode();
        void enterEditMode(const ISceneObject *object, IEditablePrimitive *editable);
        void exitEditMode();
        // Nearest vertex handle to the window-space cursor within the pick
        // radius (tie-broken by proximity to the camera), or -1 if none.
        int pickVertexHandle(const sf::Vector2i &mouse) const;
        // Project vertex `index` to window space; false if behind/off screen.
        bool vertexHandleWindowPos(std::size_t index, sf::Vector2f &out) const;
        void beginVertexDrag(int index);
        void applyVertexDrag(const sf::Vector2i &mouse);
        void endVertexDrag();
        // Re-push the current selection to the viewport renderer, which bumps
        // its selection version and forces a fresh geometry pass -- the way the
        // UI signals "the scene changed, re-trace" after an edit.
        void forceViewportRetrace();
        void syncVertexEditorField();
        // Step the selected vertex to the previous/next one (Object-panel arrows),
        // entering edit mode first if needed; syncVertexNavigator refreshes the
        // "< Point N / total >" label from the current selection.
        void navigateVertex(int direction);
        void syncVertexNavigator();
        // Bake the selected analytic primitive into an editable mesh (button in
        // the Object panel) and select the resulting mesh.
        void convertSelectionToMesh();
        // Object move-by-drag helpers.
        const ISceneObject *pickObjectAt(const sf::Vector2i &mouse);
        bool viewportPlanePoint(const sf::Vector2i &mouse, const Vector3f &planeOrigin, Vector3f &out);
        void beginObjectDrag(ISceneObject *object, const sf::Vector2i &mouse);
        void applyObjectDrag(const sf::Vector2i &mouse);
        void endObjectDrag();
        // Placement marker (Shift+right-click) helpers.
        bool computeMarker(const sf::Vector2i &mouse, Vector3f &out);
        void placeMarker(const sf::Vector2i &mouse);
        bool markerWindowPos(sf::Vector2f &out) const;
        void drawMarker(sf::RenderWindow &window) const;
        void addPrimitiveAtMarker(const std::string &type);
        void drawEditOverlay(sf::RenderWindow &window);
        void applyImport();
        void updateViewportCamera(sf::RenderWindow &window);

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

        // Vertex edit mode state. _editTarget is a mutable view of the selected
        // editable primitive; _selectedVertex / _hoverVertex index its vertices.
        bool _editMode = false;
        const ISceneObject *_editObject = nullptr;
        IEditablePrimitive *_editTarget = nullptr;
        int _selectedVertex = -1;
        int _hoverVertex = -1;
        bool _vertexDragActive = false;
        Vector3f _dragStartWorld = {0.0f, 0.0f, 0.0f};
        // Double-click-to-edit tracking (viewport).
        sf::Clock _editClickClock;
        const ISceneObject *_editClickObject = nullptr;

        // Object move: left-drag a selected object to slide it in the plane that
        // passes through its position, parallel to the screen (same math as the
        // vertex drag). _objectDragOffset preserves the grab point so the object
        // doesn't jump under the cursor.
        bool _objectDragActive = false;
        bool _objectDragMoved = false;
        ISceneObject *_objectDragTarget = nullptr;
        Vector3f _objectDragPlaneOrigin = {0.0f, 0.0f, 0.0f};
        Vector3f _objectDragOffset = {0.0f, 0.0f, 0.0f};

        // Placement marker: Shift+right-click drops a 3D point; newly added
        // primitives spawn there instead of at the origin.
        bool _markerActive = false;
        Vector3f _markerPos = {0.0f, 0.0f, 0.0f};

        // viewport
        sf::Vector2i _lastMouse;
        bool _rightMouseHeld = false;
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
