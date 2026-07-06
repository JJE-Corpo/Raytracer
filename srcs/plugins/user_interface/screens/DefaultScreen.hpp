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
        // Placement marker (Shift+right-click) helpers.
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
