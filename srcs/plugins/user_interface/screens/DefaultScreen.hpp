//
// Created by jazema on 5/16/26.
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
        void drawClusterServerOverlay(sf::RenderWindow &window);
        bool handleClusterOverlayEvent(sf::RenderWindow &window, const sf::Event &event);
        CursorType clusterOverlayCursor();
        void drawMovementIndicator(sf::RenderWindow &window);

        void updateSelectionFromClick(const sf::Vector2i &mouse);
        void updateHoverFromMouse(const sf::Vector2i &mouse);
        void clearHover();
        void syncSelectionToRenderer();
        void markViewportBvhDirty();

        // --- Vertex edit mode ------------------------------------------------
        bool anyUiCapturing();
        IEditablePrimitive *editableFromSelection() const;
        void toggleEditMode();
        void enterEditMode(const ISceneObject *object, IEditablePrimitive *editable);
        void exitEditMode();
        int pickVertexHandle(const sf::Vector2i &mouse) const;
        bool vertexHandleWindowPos(std::size_t index, sf::Vector2f &out) const;
        void beginVertexDrag(int index);
        void applyVertexDrag(const sf::Vector2i &mouse);
        void endVertexDrag();
        void forceViewportRetrace();
        void syncVertexEditorField();
        void navigateVertex(int direction);
        void syncVertexNavigator();
        void convertSelectionToMesh();
        // Object move-by-drag helpers.
        const ISceneObject *pickObjectAt(const sf::Vector2i &mouse);
        bool viewportPlanePoint(const sf::Vector2i &mouse, const Vector3f &planeOrigin, Vector3f &out);
        void beginObjectDrag(ISceneObject *object, const sf::Vector2i &mouse);
        void applyObjectDrag(const sf::Vector2i &mouse);
        void endObjectDrag();
        // Placement marker (Ctrl+right-click) helpers.
        bool computeMarker(const sf::Vector2i &mouse, Vector3f &out);
        void placeMarker(const sf::Vector2i &mouse);
        bool markerWindowPos(sf::Vector2f &out) const;
        void drawMarker(sf::RenderWindow &window) const;
        void addPrimitiveAtMarker(const std::string &type);
        void drawAxisGizmo(sf::RenderWindow &window) const;
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
        bool rayPlanePoint(const sf::Vector2i &mouse, const Vector3f &origin,
            const Vector3f &normal, Vector3f &out) const;
        bool rotationRing(int axis, std::vector<sf::Vector2f> &pts, std::vector<char> &valid) const;
        int pickRotationRing(const sf::Vector2i &mouse) const;
        void drawRotationRings(sf::RenderWindow &window) const;
        void beginRotationDrag(ISceneObject *object, int axis, const sf::Vector2i &mouse);
        void applyRotationDrag(const sf::Vector2i &mouse);
        void endRotationDrag();
        void drawScaleGizmo(sf::RenderWindow &window) const;
        void beginScaleDrag(ISceneObject *object, int axis, const sf::Vector2i &mouse);
        void applyScaleDrag(const sf::Vector2i &mouse);
        void endScaleDrag();
        // Try to grab any gizmo handle under the cursor for the active tool
        // (arrow / plane / ring / centre / view-ring) and start its drag; returns
        // true if one was grabbed, so the click doesn't fall through to selection.
        bool beginGizmoDrag(ISceneObject *object, const sf::Vector2i &mouse);
        // World axis, or the object's local (rotated) axis when _gizmoLocal is on.
        Vector3f gizmoAxisDir(int axis) const;
        // Switch the active gizmo tool and reflect it on the Object-panel buttons.
        void setGizmoTool(GizmoMode mode);
        // G / R / S switch tool, T toggles Local/World; consumes the key if used.
        bool handleGizmoShortcut(const sf::Event &event, const sf::Vector2i &mouse);
        // Live value readout shown near the gizmo while a drag is in progress.
        void drawGizmoReadout(sf::RenderWindow &window) const;
        // Planar move handles (XY / YZ / ZX squares between the arrows).
        bool gizmoPlaneHandle(int plane, sf::Vector2f &out) const;
        int pickPlaneHandle(const sf::Vector2i &mouse) const;
        void drawPlaneHandles(sf::RenderWindow &window) const;
        void beginPlaneDrag(ISceneObject *object, int plane, const sf::Vector2i &mouse);
        void applyPlaneDrag(const sf::Vector2i &mouse);
        void endPlaneDrag();
        // Uniform scale: a grabbable centre box that scales all three axes.
        bool pickUniformScale(const sf::Vector2i &mouse) const;
        void beginUniformScaleDrag(ISceneObject *object, const sf::Vector2i &mouse);
        void applyUniformScaleDrag(const sf::Vector2i &mouse);
        void endUniformScaleDrag();
        // Screen-space rotation ring (rotate about the camera-forward axis).
        bool pickViewRotation(const sf::Vector2i &mouse) const;
        void drawViewRotationRing(sf::RenderWindow &window) const;
        void beginViewRotationDrag(ISceneObject *object, const sf::Vector2i &mouse);
        void applyViewRotationDrag(const sf::Vector2i &mouse);
        void endViewRotationDrag();
        void drawEditOverlay(sf::RenderWindow &window);
        void applyImport();
        void updateViewportCamera(sf::RenderWindow &window);

        bool handleShortcut(const sf::Event &event);
        void triggerSave();
        void undoShortcut();
        void redoShortcut();
        void onSceneRestored();
        bool isKeyboardCaptured();

        const ISceneObject *pickViewportObject(const sf::Vector2i &mouse);
        void openContextMenu(const ISceneObject *object, const sf::Vector2i &mouse);
        void hideSelection();
        void groupSelection();

        void trackFlyKeys(const sf::Event &event, const sf::Vector2i &mouse);
        void setFlyKey(sf::Keyboard::Key key, bool pressed);
        void resetFlyKeys();

        void toggleMovementMode();

        bool isViewportCaptured(const sf::Vector2i &mouse);

        ICoreAccess *_coreAccess = nullptr;
        sf::Font *_font = nullptr;

        ViewMode _viewMode = ViewMode::VIEWPORT;
        bool _viewportBvhDirty = true;
        ISceneRenderer *_activeRenderer = nullptr;

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

        void applyMarketAdditions();

        std::string _exploratorResult;
        bool _exploratorJustClosed = false;
        std::function<void()> _exploratorOnClose;

        bool _loadWindowPending = false;
        std::function<void()> _loadWindowOnClose;

        ToastManager _toastManager;

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

        bool _objectDragActive = false;
        bool _objectDragMoved = false;
        ISceneObject *_objectDragTarget = nullptr;
        Vector3f _objectDragPlaneOrigin = {0.0f, 0.0f, 0.0f};
        Vector3f _objectDragOffset = {0.0f, 0.0f, 0.0f};

        // Placement marker: Ctrl+right-click drops a 3D point; newly added
        // primitives spawn there instead of at the origin.
        bool _markerActive = false;
        Vector3f _markerPos = {0.0f, 0.0f, 0.0f};

        bool _axisDragActive = false;
        bool _axisDragMoved = false;
        ISceneObject *_axisDragTarget = nullptr;
        int _axisDragAxis = -1;
        Vector3f _axisDragObjStart = {0.0f, 0.0f, 0.0f};
        Vector3f _axisDragDir = {0.0f, 0.0f, 0.0f};
        float _axisDragGrabT = 0.0f;

        bool _rotDragActive = false;
        bool _rotDragMoved = false;
        bool _rotDragValid = false;
        ISceneObject *_rotDragTarget = nullptr;
        int _rotDragAxis = -1;
        Vector3f _rotDragStartRot = {0.0f, 0.0f, 0.0f};
        Vector3f _rotDragObjPos = {0.0f, 0.0f, 0.0f};
        Vector3f _rotDragAxisN = {0.0f, 0.0f, 0.0f};
        Vector3f _rotDragGrabVec = {0.0f, 0.0f, 0.0f};

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
        // Gizmo axes follow the object's rotation (local) instead of the world.
        bool _gizmoLocal = false;
        // Live "+2.35 X" style readout while a gizmo drag is running ("" = none).
        std::string _gizmoReadout;

        // Planar-move drag (XY/YZ/ZX): the object slides in a fixed plane.
        bool _planeDragActive = false;
        bool _planeDragMoved = false;
        ISceneObject *_planeDragTarget = nullptr;
        int _planeDragPlane = -1;
        Vector3f _planeDragNormal = {0.0f, 0.0f, 0.0f};
        Vector3f _planeDragObjStart = {0.0f, 0.0f, 0.0f};
        Vector3f _planeDragOffset = {0.0f, 0.0f, 0.0f};

        // Uniform-scale drag (centre box): all three axes scale together by the
        // ratio of the cursor's screen distance from the object centre.
        bool _uscaleDragActive = false;
        bool _uscaleDragMoved = false;
        ISceneObject *_uscaleDragTarget = nullptr;
        Vector3f _uscaleStartScale = {1.0f, 1.0f, 1.0f};
        sf::Vector2f _uscaleCenter = {0.0f, 0.0f};
        float _uscaleGrabDist = 0.0f;

        // Screen-space rotation drag (rotate about the camera-forward axis).
        bool _viewRotActive = false;
        bool _viewRotMoved = false;
        bool _viewRotValid = false;
        ISceneObject *_viewRotTarget = nullptr;
        Vector3f _viewRotStartRot = {0.0f, 0.0f, 0.0f};
        Vector3f _viewRotObjPos = {0.0f, 0.0f, 0.0f};
        Vector3f _viewRotAxis = {0.0f, 0.0f, 0.0f};
        Vector3f _viewRotGrabVec = {0.0f, 0.0f, 0.0f};

        // viewport
        sf::Vector2i _lastMouse;
        bool _rightMouseHeld = false;
        sf::Vector2i _rightPressMouse;
        bool _rightPressInViewport = false;
        bool _rightDragged = false;
        float _cameraSpeed = 6.0f;
        sf::Clock _frameClock;
        bool _keyForward = false;
        bool _keyBack = false;
        bool _keyLeft = false;
        bool _keyRight = false;
        bool _keyUp = false;
        bool _keyDown = false;
        bool _movementMode = false;

        public:
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
