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
#include <vector>

#include <SFML/System/Clock.hpp>
#include <SFML/Window/Keyboard.hpp>

#include "AScreen.hpp"
#include "../components/menu/MenuBar.hpp"
#include "../components/menu/ContextMenu.hpp"
#include "../components/ResizeHandle.hpp"
#include "../panels/CameraPanel.hpp"
#include "../panels/HierarchyPanel.hpp"
#include "../panels/MaterialPanel.hpp"
#include "../panels/ClusterServerPanel.hpp"
#include "../panels/ObjectPanel.hpp"
#include "../panels/RendererPanel.hpp"
#include "../panels/SidebarStack.hpp"
#include "../toast/ToastManager.hpp"
#include "../windows/ExploratorWindow.hpp"
#include "../windows/JoinClusterWindow.hpp"
#include "../windows/LoadWindow.hpp"
#include "../windows/MarketWindow.hpp"
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

        // Which transform gizmo is active on the selected object. Chosen from the
        // Object panel's Move / Rotate / Scale buttons (values match those: 0/1/2).
        enum class GizmoMode
        {
            MOVE = 0,
            ROTATE = 1,
            SCALE = 2
        };

        void setupSidebarSection(SidebarStack::Slot slot, const std::string &id, const std::string &title,
            Component *content, std::function<void(float, float, float)> layoutContent,
            std::function<float()> contentHeight);

        void layoutSidebarResize(const sf::RenderWindow &window);
        void refreshSidebarVisibility();
        void drawRenderer(sf::RenderWindow &window, ISceneRenderer *renderer);
        // Host-side cluster HUD (top-right); no-ops unless this node is a server.
        void drawClusterServerOverlay(sf::RenderWindow &window);
        // Route a mouse event to the cluster HUD's minimize/expand toggle; returns
        // true when the toggle consumed the click so it never reaches the scene.
        bool handleClusterOverlayEvent(sf::RenderWindow &window, const sf::Event &event);
        // The cursor the cluster HUD wants (move over its header, hand over the
        // toggle), or ARROW when not hosting or not hovering it.
        CursorType clusterOverlayCursor();
        // Persistent badge in the viewport's bottom-left corner, drawn only while
        // movement mode is on, so the active camera controls are always visible
        // (the toggle toast is transient).
        void drawMovementIndicator(sf::RenderWindow &window);

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
        // Small X/Y/Z orientation compass drawn in the viewport's top-right.
        void drawAxisGizmo(sf::RenderWindow &window) const;
        // Move gizmo: three axis arrows on the selected object; dragging one
        // slides the object along that single world axis.
        ISceneObject *singleSelectedObject() const;
        bool projectToViewport(const Vector3f &point, sf::Vector2f &out) const;
        bool gizmoArrow(int axis, sf::Vector2f &origin, sf::Vector2f &tip) const;
        int pickGizmoAxis(const sf::Vector2i &mouse) const;
        void drawMoveGizmo(sf::RenderWindow &window) const;
        bool axisParamFromMouse(const sf::Vector2i &mouse, const Vector3f &axisOrigin,
            const Vector3f &axisDir, float &t) const;
        void beginAxisDrag(ISceneObject *object, int axis, const sf::Vector2i &mouse);
        void applyAxisDrag(const sf::Vector2i &mouse);
        void endAxisDrag();
        // Rotation gizmo: three rings around the selected object; dragging one
        // rotates the object around that axis.
        bool rayPlanePoint(const sf::Vector2i &mouse, const Vector3f &origin,
            const Vector3f &normal, Vector3f &out) const;
        bool rotationRing(int axis, std::vector<sf::Vector2f> &pts, std::vector<char> &valid) const;
        int pickRotationRing(const sf::Vector2i &mouse) const;
        void drawRotationRings(sf::RenderWindow &window) const;
        void beginRotationDrag(ISceneObject *object, int axis, const sf::Vector2i &mouse);
        void applyRotationDrag(const sf::Vector2i &mouse);
        void endRotationDrag();
        // Scale gizmo: reuses the move-arrow geometry (box-tipped), dragging an
        // axis scales the object along that axis.
        void drawScaleGizmo(sf::RenderWindow &window) const;
        void beginScaleDrag(ISceneObject *object, int axis, const sf::Vector2i &mouse);
        void applyScaleDrag(const sf::Vector2i &mouse);
        void endScaleDrag();
        void drawEditOverlay(sf::RenderWindow &window);
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
        ClusterServerPanel _clusterServerPanel;

        // Windows
        bool _isNewScene = false;
        JoinClusterWindow _joinClusterWindow;
        LoadWindow _loadWindow;
        ExploratorWindow _exploratorWindow;
        MarketWindow _marketWindow;

        // Apply any "Add" clicks queued by the (threaded) market window into the
        // current scene's material set. Runs on the main thread.
        void applyMarketAdditions();

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

        // Move-gizmo drag: an axis arrow was grabbed; the object slides along
        // _axisDragDir by the change in the closest-point parameter of the mouse
        // ray relative to that axis line (see applyAxisDrag).
        bool _axisDragActive = false;
        bool _axisDragMoved = false;
        ISceneObject *_axisDragTarget = nullptr;
        int _axisDragAxis = -1;
        Vector3f _axisDragObjStart = {0.0f, 0.0f, 0.0f};
        Vector3f _axisDragDir = {0.0f, 0.0f, 0.0f};
        float _axisDragGrabT = 0.0f;

        // Rotation-gizmo drag: a ring was grabbed; the object's Euler angle for
        // that axis follows the signed angle swept by the mouse in the ring's
        // plane relative to where it was grabbed (see applyRotationDrag).
        bool _rotDragActive = false;
        bool _rotDragMoved = false;
        bool _rotDragValid = false;
        ISceneObject *_rotDragTarget = nullptr;
        int _rotDragAxis = -1;
        Vector3f _rotDragStartRot = {0.0f, 0.0f, 0.0f};
        Vector3f _rotDragObjPos = {0.0f, 0.0f, 0.0f};
        Vector3f _rotDragAxisN = {0.0f, 0.0f, 0.0f};
        Vector3f _rotDragGrabVec = {0.0f, 0.0f, 0.0f};

        // Scale-gizmo drag: the object's scale for _scaleDragAxis is multiplied by
        // how far the mouse moved along the axis relative to where it was grabbed.
        bool _scaleDragActive = false;
        bool _scaleDragMoved = false;
        ISceneObject *_scaleDragTarget = nullptr;
        int _scaleDragAxis = -1;
        Vector3f _scaleDragStartScale = {1.0f, 1.0f, 1.0f};
        Vector3f _scaleDragObjStart = {0.0f, 0.0f, 0.0f};
        Vector3f _scaleDragDir = {0.0f, 0.0f, 0.0f};
        float _scaleDragGrabT = 0.0f;

        // Active transform gizmo (Move / Rotate / Scale).
        GizmoMode _gizmoMode = GizmoMode::MOVE;

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
