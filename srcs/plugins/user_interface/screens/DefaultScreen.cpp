//
// Created by jazema on 5/16/26.
//

#include "DefaultScreen.hpp"
#include "../../../common/scene/ILight.hpp"
#include "../../../common/Intersection.hpp"
#include "../../../common/scene/IPrimitive.hpp"
#include "../../../common/scene/IEditablePrimitive.hpp"
#include "../../../common/ISelectionAwareRenderer.hpp"
#include "../../../common/scene/IScene.hpp"
#include "../../../common/Axis.hpp"
#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdio>
#include <limits>
#include <string>

#include "../EventRouter.hpp"
#include "../components/menu/Menu.hpp"
#include "../Theme.hpp"
#include "../ViewportHelper.hpp"

namespace rc
{
    constexpr float MENU_HEIGHT = 28.0f;
    constexpr float SIDEBAR_MIN = 220.0f;   // narrowest the sidebar may be dragged
    constexpr float VIEWPORT_MIN = 360.0f;  // space always kept for the viewport
    constexpr float RIGHT_CLICK_DRAG = 5.0f;

    void DefaultScreen::setCoreAccess(ICoreAccess *coreAccess)
    {
        this->_coreAccess = coreAccess;
    }

    void DefaultScreen::setFont(sf::Font &font)
    {
        this->_font = &font;
    }

    void DefaultScreen::buildUI()
    {
        this->_viewportBvhDirty = true;
        this->_toastManager.setFont(*this->_font);

        this->_exploratorWindow.setSelectedEntry({".json"});

        this->_rendererPanel.setFont(*this->_font);
        this->_clusterServerPanel.setFont(*this->_font);
        this->_rendererPanel.closeRenderCallback = [this]
        {
            if (this->_coreAccess && this->_coreAccess->getRenderer()->isRendering())
                this->_coreAccess->getRenderer()->stopRendering();
            while (this->_coreAccess && this->_coreAccess->getRenderer()->isRendering())
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            this->_viewMode = ViewMode::VIEWPORT;
        };
        this->_rendererPanel.isRenderViewCallback = [this]
        {
            return (this->_viewMode == ViewMode::RENDERING);
        };
        this->_rendererPanel.getRendererCommentCallback = [this]() -> std::string
        {
            if (this->_coreAccess->getRenderer() == nullptr)
                return ("Could not fetch comment from renderer");
            if (this->_coreAccess->getRenderer()->isRendering())
                return ("Rendering... (Sample " + std::to_string(this->_coreAccess->getRenderer()->getCurrentSample()) + "/" + std::to_string(this->_coreAccess->getScene()->getCamera().getSamplesPerPixel()) + ")");
            return ("Render finished !");
        };

        if (this->_coreAccess->getViewportRenderer() == nullptr)
        {
            this->_toastManager.push("Missing viewport library", "Cannot find the viewport renderer files.", ToastType::ERROR);
        }

        Menu sceneMenu;
        sceneMenu.setLabel("Scene");

        MenuItem newItem;
        newItem.setLabel("New");
        newItem.onClick = [&]() {
            if (this->_exploratorJustClosed == true)
            {
                this->_toastManager.push("Please close the current explorer window first", "An explorer window is already open.", ToastType::ERROR);
                return;
            }

            this->_isNewScene = true;
            this->_coreAccess->clearScene();
            this->markViewportBvhDirty();

            this->_toastManager.push("New scene", "A new scene has been created.", ToastType::SUCCESS);
        };

        MenuItem openItem;
        openItem.setLabel("Open");
        openItem.onClick = [&]() {
            if (this->_exploratorJustClosed == true)
            {
                this->_toastManager.push("Please close the current explorer window first", "An explorer window is already open.", ToastType::ERROR);
                return;
            }

            try
            {
                this->_exploratorResult = "";
                this->_exploratorJustClosed = true;

                this->_exploratorOnClose = [&]()
                {
                    if (!this->_exploratorResult.empty())
                    {
                        try
                        {
                            this->_coreAccess->loadScene(this->_exploratorResult);
                            this->markViewportBvhDirty();
                            this->_toastManager.push("Scene loaded", "Scene loaded successfully.", ToastType::SUCCESS);
                        }
                        catch (const std::exception &e)
                        {
                            this->_toastManager.push("Error loading scene", e.what(), ToastType::ERROR);
                            return;
                        }
                    }
                    else
                    {
                        this->_toastManager.push("No file selected", "No file was selected.", ToastType::INFO);
                    }
                };

                this->_exploratorWindow.create(*this->_font, ExploratorMode::OPEN, this->_exploratorResult);
            }
            catch(const std::exception& e)
            {
                this->_toastManager.push("Error loading scene", e.what(), ToastType::ERROR);
            }
        };

        MenuItem saveItem;
        saveItem.setLabel("Save");
        saveItem.onClick = [&]() {
            this->triggerSave();
        };

        MenuItem saveAsItem;
        saveAsItem.setLabel("Save as");
        saveAsItem.onClick = [&]() {
            if (this->_exploratorJustClosed == true)
            {
                this->_toastManager.push("Please close the current explorer window first", "An explorer window is already open.", ToastType::ERROR);
                return;
            }

            try
            {
                this->_exploratorResult = "";
                this->_exploratorJustClosed = true;

                this->_exploratorOnClose = [&]()
                {
                    if (!this->_exploratorResult.empty())
                    {
                        try
                        {
                            this->_coreAccess->saveScene(this->_exploratorResult);
                            this->_toastManager.push("Scene saved", "Scene saved successfully.", ToastType::SUCCESS);
                        }
                        catch (const std::exception &e)
                        {
                            this->_toastManager.push("Error saving scene", e.what(), ToastType::ERROR);
                        }
                    }
                    else
                    {
                        this->_toastManager.push("No file selected", "No file was selected.", ToastType::INFO);
                    }
                };

                this->_exploratorWindow.create(*this->_font, ExploratorMode::SAVE, this->_exploratorResult);
            }
            catch(const std::exception& e)
            {
                this->_toastManager.push("Error loading scene", e.what(), ToastType::ERROR);
            }
        };

        MenuItem importItem;
        importItem.setLabel("Import");
        importItem.onClick = [&]() {
            if (this->_exploratorJustClosed == true)
            {
                this->_toastManager.push("Please close the current explorer window first", "An explorer window is already open.", ToastType::ERROR);
                return;
            }

            try
            {
                this->_exploratorResult = "";
                this->_exploratorJustClosed = true;

                this->_exploratorOnClose = [&]()
                {
                    if (!this->_exploratorResult.empty())
                    {
                        try
                        {
                            this->_loadWindow.create(*this->_font, *this->_coreAccess->getScene(), this->_coreAccess->loadNewScene(this->_exploratorResult));
                            this->_loadWindowPending = true;
                        }
                        catch (const std::exception &e)
                        {
                            this->_toastManager.push("Error importing scene", e.what(), ToastType::ERROR);
                            return;
                        }
                    }
                    else
                    {
                        this->_toastManager.push("No file selected", "No file was selected.", ToastType::INFO);
                    }
                };

                this->_exploratorWindow.create(*this->_font, ExploratorMode::OPEN, this->_exploratorResult);
            }
            catch(const std::exception& e)
            {
                this->_toastManager.push("Error loading scene", e.what(), ToastType::ERROR);
            }
        };

        MenuItem exportItem;
        exportItem.setLabel("Export (.obj)");
        exportItem.onClick = [&] {
            // todo
        };

        sceneMenu.items = { newItem, openItem, saveItem, saveAsItem, importItem, exportItem };

        Menu addMenu;
        addMenu.setLabel("Add");

        MenuItem newPlane;
        newPlane.setLabel("Plane");
        newPlane.onClick = [&]
        {
            try
            {
                this->addPrimitiveAtMarker("plane");
                this->markViewportBvhDirty();
                this->_toastManager.push("Plane added", "A new plane primitive has been added to the scene.", ToastType::SUCCESS);
            }
            catch (const std::exception &e)
            {
                this->_toastManager.push("Error creating plane", e.what(), ToastType::ERROR);
            }
        };

        MenuItem newSphere;
        newSphere.setLabel("Sphere");
        newSphere.onClick = [&]
        {
            try
            {
                this->addPrimitiveAtMarker("sphere");
                this->markViewportBvhDirty();
                this->_toastManager.push("Sphere added", "A new sphere primitive has been added to the scene.", ToastType::SUCCESS);
            }
            catch (const std::exception &e)
            {
                this->_toastManager.push("Error creating sphere", e.what(), ToastType::ERROR);
            }
        };

        MenuItem newCylinder;
        newCylinder.setLabel("Cylinder");
        newCylinder.onClick = [&]
        {
            try
            {
                this->addPrimitiveAtMarker("cylinder");
                this->markViewportBvhDirty();
                this->_toastManager.push("Cylinder added", "A new cylinder primitive has been added to the scene.", ToastType::SUCCESS);
            }
            catch (const std::exception &e)
            {
                this->_toastManager.push("Error creating cylinder", e.what(), ToastType::ERROR);
            }
        };

        MenuItem newCone;
        newCone.setLabel("Cone");
        newCone.onClick = [&]
        {
            try
            {
                this->addPrimitiveAtMarker("cone");
                this->markViewportBvhDirty();
                this->_toastManager.push("Cone added", "A new cone primitive has been added to the scene.", ToastType::SUCCESS);
            }
            catch (const std::exception &e)
            {
                this->_toastManager.push("Error creating cone", e.what(), ToastType::ERROR);
            }
        };

        MenuItem newTriangle;
        newTriangle.setLabel("Triangle");
        newTriangle.onClick = [&]
        {
            try
            {
                this->addPrimitiveAtMarker("triangle");
                this->markViewportBvhDirty();
                this->_toastManager.push("Triangle added", "A new triangle primitive has been added to the scene.", ToastType::SUCCESS);
            }
            catch (const std::exception &e)
            {
                this->_toastManager.push("Error creating triangle", e.what(), ToastType::ERROR);
            }
        };

        MenuItem newCube;
        newCube.setLabel("Cube");
        newCube.onClick = [&]
        {
            try
            {
                this->addPrimitiveAtMarker("cube");
                this->markViewportBvhDirty();
                this->_toastManager.push("Cube added", "A new cube primitive has been added to the scene.", ToastType::SUCCESS);
            }
            catch (const std::exception &e)
            {
                this->_toastManager.push("Error creating cube", e.what(), ToastType::ERROR);
            }
        };

        MenuItem newFractal;
        newFractal.setLabel("Fractal");
        newFractal.onClick = [&]
        {
            try
            {
                this->addPrimitiveAtMarker("fractal");
                this->markViewportBvhDirty();
                this->_toastManager.push("Fractal added", "A new fractal primitive has been added to the scene.", ToastType::SUCCESS);
            }
            catch (const std::exception &e)
            {
                this->_toastManager.push("Error creating fractal", e.what(), ToastType::ERROR);
            }
        };

        MenuItem newTanglecube;
        newTanglecube.setLabel("Tanglecube");
        newTanglecube.onClick = [&]
        {
            try
            {
                this->addPrimitiveAtMarker("tanglecube");
                this->markViewportBvhDirty();
                this->_toastManager.push("Tanglecube added", "A new tanglecube primitive has been added to the scene.", ToastType::SUCCESS);
            }
            catch (const std::exception &e)
            {
                this->_toastManager.push("Error creating tanglecube", e.what(), ToastType::ERROR);
            }
        };

        MenuItem newTorus;
        newTorus.setLabel("Torus");
        newTorus.onClick = [&]
        {
            try
            {
                this->addPrimitiveAtMarker("torus");
                this->markViewportBvhDirty();
                this->_toastManager.push("Torus added", "A new torus primitive has been added to the scene.", ToastType::SUCCESS);
            }
            catch (const std::exception &e)
            {
                this->_toastManager.push("Error creating torus", e.what(), ToastType::ERROR);
            }
        };

        MenuItem newPointLight; // todo voir si on fait une sous section lights
        newPointLight.setLabel("Point light");
        newPointLight.onClick = [&]
        {
            try
            {
                this->_coreAccess->getScene()->addDefaultLight("point_light");
                this->markViewportBvhDirty();
                this->_toastManager.push("Point light added", "A new point light has been added to the scene.", ToastType::SUCCESS);
            }
            catch (const std::exception &e)
            {
                this->_toastManager.push("Error creating point light", e.what(), ToastType::ERROR);
            }
        };

        MenuItem newDirectionalLight; // todo voir si on fait une sous section lights
        newDirectionalLight.setLabel("Directional light");
        newDirectionalLight.onClick = [&]
        {
            try
            {
                this->_coreAccess->getScene()->addDefaultLight("directional_light");
                this->markViewportBvhDirty();
                this->_toastManager.push("Directional light added", "A new directional light has been added to the scene.", ToastType::SUCCESS);
            }
            catch (const std::exception &e)
            {
                this->_toastManager.push("Error creating directional light", e.what(), ToastType::ERROR);
            }
        };

        MenuItem newGroup;
        newGroup.setLabel("Group");
        newGroup.onClick = [&]
        {
            try
            {
                this->_coreAccess->getScene()->addGroup();
                this->markViewportBvhDirty();
                this->_toastManager.push("Group added", "Drag objects onto it in the hierarchy to nest them.", ToastType::SUCCESS);
            }
            catch (const std::exception &e)
            {
                this->_toastManager.push("Error creating group", e.what(), ToastType::ERROR);
            }
        };

        addMenu.items = { newGroup, newPlane, newSphere, newCylinder, newCone, newTriangle, newCube, newFractal, newTanglecube, newTorus, newPointLight, newDirectionalLight };

        Menu renderMenu;
        renderMenu.setLabel("Render");

        MenuItem renderRender;
        renderRender.setLabel("Render frame");
        renderRender.onClick = [&]
        {
            if (this->_coreAccess->getRenderer()->isRendering())
            {
                this->_toastManager.push("Cannot render", "Already rendering a frame", ToastType::ERROR);
                return;
            }
            this->_toastManager.push("Rendering..", "Started rendering scene..", ToastType::INFO);
            this->_coreAccess->requestRender();
            this->_viewMode = ViewMode::RENDERING;
        };

        renderMenu.items = { renderRender };

        Menu clusterMenu;
        clusterMenu.setLabel("Clustering");

        MenuItem clusterHost;
        clusterHost.setLabel("Start server");
        clusterHost.onClick = [&]
        {
            try
            {
                this->_coreAccess->getClusterModule()->startServer(this->_coreAccess->getScene());
                this->_toastManager.push("Server started !", "Clients can now join. See the cluster panel (top-right) for the port and connected clients.", ToastType::SUCCESS);
            }
            catch (std::exception &e)
            {
                this->_toastManager.push("Cannot create server", e.what(), ToastType::ERROR);
            }
        };

        MenuItem clusterConnect;
        clusterConnect.setLabel("Connect");
        clusterConnect.onClick = [&]
        {
            this->_joinClusterWindow.create(*this->_font);
        };

        clusterMenu.items = { clusterHost, clusterConnect };

        Menu materialsMenu;
        materialsMenu.setLabel("Materials");

        MenuItem openMarket;
        openMarket.setLabel("Material market");
        openMarket.onClick = [&]
        {
            if (this->_marketWindow.running)
            {
                this->_toastManager.push("Market already open", "The material market window is already open.", ToastType::INFO);
                return;
            }
            this->_marketWindow.create(*this->_font);
        };

        materialsMenu.items = { openMarket };

        this->_joinClusterWindow.windowCallback = [this](std::string ip, size_t port)
        {
            try
            {
                this->_coreAccess->getClusterModule()->joinCluster(ip, port);
                this->onClusterJoined(this->_coreAccess->getClusterModule()->getClusterClient());
            }
            catch (std::exception &e)
            {
                this->_toastManager.push("Cannot join cluster", e.what(), ToastType::ERROR);
            }
        };

        this->_menuBar.menus.push_back(sceneMenu);
        this->_menuBar.menus.push_back(addMenu);
        this->_menuBar.menus.push_back(renderMenu);
        this->_menuBar.menus.push_back(clusterMenu);
        this->_menuBar.menus.push_back(materialsMenu);
        this->_menuBar.setFont(*this->_font);

        this->_contextMenu.setFont(*this->_font);

        this->_hierarchyPanel.setFont(*this->_font);
        this->_hierarchyPanel.setOnItemHideRequest([this](const ISceneObject *payload)
        {
            IScene *scene = this->_coreAccess ? this->_coreAccess->getScene() : nullptr;

            if (!scene || !payload)
                return;
            auto *wp = const_cast<ISceneObject *>(payload);
            if (!wp)
                return;
            wp->setHidden(!wp->isHidden());

            this->markViewportBvhDirty();
            if (this->_coreAccess && this->_coreAccess->getViewportRenderer())
            {
                auto *selection_renderer = dynamic_cast<ISelectionAwareRenderer *>(this->_coreAccess->getViewportRenderer());
                if (selection_renderer)
                    selection_renderer->setSelection(this->_hierarchyPanel.getSelection());
            }
        });
        this->_hierarchyPanel.setOnReparentRequest([this](const ISceneObject *child, const ISceneObject *newParent, int index)
        {
            IScene *scene = this->_coreAccess ? this->_coreAccess->getScene() : nullptr;

            if (!scene || !child)
                return;
            scene->reparent(const_cast<ISceneObject *>(child), const_cast<ISceneObject *>(newParent), index);
            this->markViewportBvhDirty();
        });
        this->_hierarchyPanel.setOnItemDeleteRequest([this](const ISceneObject *payload)
        {
            IScene *scene = this->_coreAccess ? this->_coreAccess->getScene() : nullptr;

            if (!scene || !payload)
                return;
            const std::string name = payload->getName();
            scene->removeObject(const_cast<ISceneObject *>(payload));
            this->markViewportBvhDirty();
            this->_toastManager.push("Object deleted", name + " has been removed from the scene.", ToastType::SUCCESS);
        });

        this->_cameraPanel.setFont(*this->_font);
        this->_objectPanel.setFont(*this->_font);
        this->_objectPanel.onSceneMutated = [this]
        {
            this->markViewportBvhDirty();
            this->forceViewportRetrace();
        };
        this->_objectPanel.onVertexEdit = [this](Axis axis, float value) -> bool
        {
            if (!this->_editMode || !this->_editTarget || this->_selectedVertex < 0)
                return (false);
            Vector3f vertex = this->_editTarget->getVertex(static_cast<std::size_t>(this->_selectedVertex));
            if (axis == Axis::X) vertex.x = value;
            else if (axis == Axis::Y) vertex.y = value;
            else if (axis == Axis::Z) vertex.z = value;
            this->_editTarget->setVertex(static_cast<std::size_t>(this->_selectedVertex), vertex);
            this->_editTarget->onGeometryChanged();
            this->markViewportBvhDirty();
            this->forceViewportRetrace();
            return (true);
        };
        this->_objectPanel.onVertexScale = [this](float factor) -> bool
        {
            if (!this->_editMode || !this->_editTarget || this->_selectedVertex < 0)
                return (false);
            const std::size_t count = this->_editTarget->getVertexCount();
            if (count == 0)
                return (false);
            Vector3f centroid = {0.0f, 0.0f, 0.0f};
            for (std::size_t i = 0; i < count; ++i)
                centroid = centroid + this->_editTarget->getVertex(i);
            centroid = centroid * (1.0f / static_cast<float>(count));

            const Vector3f vertex = this->_editTarget->getVertex(static_cast<std::size_t>(this->_selectedVertex));
            const Vector3f moved = centroid + (vertex - centroid) * factor;
            this->_editTarget->setVertex(static_cast<std::size_t>(this->_selectedVertex), moved);
            this->_editTarget->onGeometryChanged();
            this->markViewportBvhDirty();
            this->forceViewportRetrace();
            this->syncVertexEditorField();
            return (true);
        };
        this->_objectPanel.onVertexNavigate = [this](int direction)
        {
            this->navigateVertex(direction);
        };
        this->_objectPanel.onConvertToMesh = [this]
        {
            this->convertSelectionToMesh();
        };
        this->_objectPanel.onGizmoModeChanged = [this](int mode)
        {
            this->_gizmoMode = static_cast<GizmoMode>(mode);
            this->_objectPanel.setGizmoMode(mode);
        };
        this->_objectPanel.setGizmoMode(static_cast<int>(this->_gizmoMode));
        this->_objectPanel.onGizmoSpaceChanged = [this](bool local)
        {
            this->_gizmoLocal = local;
        };
        this->_objectPanel.setGizmoSpace(this->_gizmoLocal);
        this->_materialPanel.setFont(*this->_font);

        this->_sidebarResize.onResize = [this](float width)
        {
            this->_sidebarWidth = width;
        };

        this->setupSidebarSection(SidebarStack::HIERARCHY, "hierarchy", "Hierarchy", &this->_hierarchyPanel,
            [this](float x, float y, float w) { this->_hierarchyPanel.layout(x, y, w); },
            [this] { return this->_hierarchyPanel.height; });
        this->_sidebar.section(SidebarStack::HIERARCHY).padX = 0.f;
        this->setupSidebarSection(SidebarStack::CAMERA, "camera", "Camera", &this->_cameraPanel,
            [this](float x, float y, float w) { this->_cameraPanel.layout(x, y, w); },
            [this] { return this->_cameraPanel.height; });
        this->setupSidebarSection(SidebarStack::OBJECT, "object", "Object", &this->_objectPanel,
            [this](float x, float y, float w) { this->_objectPanel.layout(x, y, w); },
            [this] { return this->_objectPanel.height; });
        this->setupSidebarSection(SidebarStack::MATERIAL, "material", "Material", &this->_materialPanel,
            [this](float x, float y, float w) { this->_materialPanel.layout(x, y, w); },
            [this] { return this->_materialPanel.height; });
    }

    void DefaultScreen::setupSidebarSection(SidebarStack::Slot slot, const std::string &id, const std::string &title,
        Component *content, std::function<void(float, float, float)> layoutContent, std::function<float()> contentHeight)
    {
        Section &section = this->_sidebar.section(slot);
        section.id = id;
        section.setTitle(title);
        section.setFont(*this->_font);
        section.body.content = content;
        section.body.layoutContent = std::move(layoutContent);
        section.body.contentHeight = std::move(contentHeight);
    }

    void DefaultScreen::update(sf::RenderWindow &window)
    {
        sf::Vector2i mouse = sf::Mouse::getPosition(window);
        CursorType cursorType = this->_menuBar.getCursor();

        this->applyMarketAdditions();

        if (this->_exploratorJustClosed && !this->_exploratorWindow.running)
        {
            this->_exploratorJustClosed = false;
            this->_exploratorOnClose();
        }

        if (this->_loadWindowPending && !this->_loadWindow.running)
        {
            this->_loadWindowPending = false;
            this->applyImport();
        }

        if (this->_joinClusterWindow.running)
            return;

        if (this->_exploratorWindow.running)
            return;

        if (this->_loadWindow.running)
            return;

        if (this->_marketWindow.running)
            return;

        this->_contextMenu.update(mouse);

        this->_menuBar.update(mouse);

        if (this->_menuBar.isOpen())
        {
            this->applyCursor(window, this->_menuBar.getCursor());
            return;
        }

        this->_rendererPanel.update(mouse);
        if (cursorType == CursorType::ARROW && this->_rendererPanel.getCursor() != CursorType::ARROW)
            cursorType = this->_rendererPanel.getCursor();

        const CursorType clusterCursor = this->clusterOverlayCursor();
        if (clusterCursor != CursorType::ARROW)
            cursorType = clusterCursor;

        if (this->_viewMode == ViewMode::RENDERING)
        {
            this->applyCursor(window, cursorType);
            return;
        }

        this->updateViewportCamera(window);

        this->layoutSidebarResize(window);

        IScene *liveScene = this->_coreAccess ? this->_coreAccess->getScene() : nullptr;
        if (liveScene != this->_lastHistoryScene)
        {
            this->_hierarchyPanel.applyViewportSelection({});
            this->clearHover();
            this->syncSelectionToRenderer();
            if (this->_coreAccess)
                this->_coreAccess->historyReset();
            this->_lastHistoryScene = liveScene;
        }

        this->_hierarchyPanel.setScene(this->_coreAccess ? this->_coreAccess->getScene() : nullptr);

        if (this->_editMode && this->editableFromSelection() != this->_editTarget)
            this->exitEditMode();

        this->refreshSidebarVisibility();
        this->_sidebar.layout(this->_sidebarWidth, MENU_HEIGHT, static_cast<float>(window.getSize().y));
        this->_sidebar.update(mouse);

        if (cursorType == CursorType::ARROW && this->_sidebar.getCursor() != CursorType::ARROW)
            cursorType = this->_sidebar.getCursor();

        bool inViewport = this->_viewMode == ViewMode::VIEWPORT && cursorType == CursorType::ARROW && this->_rendererPanel.viewportBounds.contains(mouse);

        if (inViewport)
            cursorType = CursorType::CROSS;

        this->_sidebarResize.update(mouse);
        if (this->_sidebarResize.getCursor() != CursorType::ARROW)
            cursorType = this->_sidebarResize.getCursor();

        if (clusterCursor != CursorType::ARROW)
            cursorType = clusterCursor;

        this->applyCursor(window, cursorType);

        this->_toastManager.update();
    }

    void DefaultScreen::draw(sf::RenderWindow &window)
    {
        this->layoutSidebarResize(window);

        auto size = window.getSize();

        sf::RectangleShape panel;
        panel.setPosition({0, MENU_HEIGHT});
        panel.setSize({this->_sidebarWidth, (float)size.y - MENU_HEIGHT});
        panel.setFillColor(theme::BG_PANEL);
        window.draw(panel);

        if (this->_viewMode != ViewMode::RENDERING)
        {
            this->refreshSidebarVisibility();
            this->_sidebar.layout(this->_sidebarWidth, MENU_HEIGHT, static_cast<float>(size.y));
            this->_sidebar.draw(window);

            window.draw(this->_sidebarResize);
        }

        if (this->_activeRenderer)
            this->drawRenderer(window, this->_activeRenderer);

        if (this->_viewMode == ViewMode::VIEWPORT)
        {
            this->drawEditOverlay(window);
            switch (this->_gizmoMode)
            {
                case GizmoMode::MOVE:
                    this->drawPlaneHandles(window);
                    this->drawMoveGizmo(window);
                    break;
                case GizmoMode::ROTATE:
                    this->drawViewRotationRing(window);
                    this->drawRotationRings(window);
                    break;
                case GizmoMode::SCALE:
                    this->drawScaleGizmo(window);
                    break;
            }
            this->drawGizmoReadout(window);
            this->drawMarker(window);
            this->drawAxisGizmo(window);
        }
        if (this->_movementMode && this->_viewMode == ViewMode::VIEWPORT)
            this->drawMovementIndicator(window);

        this->drawClusterServerOverlay(window);

        this->_menuBar.layout(static_cast<float>(window.getSize().x));
        window.draw(this->_menuBar);

        window.draw(this->_contextMenu);

        this->_toastManager.draw(window);
    }

    void DefaultScreen::drawRenderer(sf::RenderWindow &window, ISceneRenderer *renderer)
    {
        sf::Vector2u windowSize = window.getSize();
        this->_rendererPanel.layout(this->_sidebarWidth, MENU_HEIGHT, static_cast<float>(windowSize.x) - this->_sidebarWidth, static_cast<float>(windowSize.y) - MENU_HEIGHT);
        this->_rendererPanel.updateRender(renderer->getRender());
        window.draw(this->_rendererPanel);
    }

    void DefaultScreen::drawClusterServerOverlay(sf::RenderWindow &window)
    {
        if (this->_coreAccess == nullptr)
            return;

        IClusterModule *clusterModule = this->_coreAccess->getClusterModule();
        if (clusterModule == nullptr || clusterModule->getClusterMode() != ClusterMode::SERVER)
            return;

        IClusterServer *server = clusterModule->getClusterServer();
        if (server == nullptr)
            return;

        ISceneRenderer *renderer = this->_coreAccess->getRenderer();
        const bool rendering = renderer != nullptr && renderer->isRendering();
        const int sample = renderer != nullptr ? renderer->getCurrentSample() : -1;

        this->_clusterServerPanel.draw(window, server, rendering, sample,
            static_cast<float>(window.getSize().x), MENU_HEIGHT + 10.f, this->_sidebarWidth);
    }

    bool DefaultScreen::handleClusterOverlayEvent(sf::RenderWindow &window, const sf::Event &event)
    {
        if (this->_coreAccess == nullptr)
            return (false);

        IClusterModule *clusterModule = this->_coreAccess->getClusterModule();
        if (clusterModule == nullptr || clusterModule->getClusterMode() != ClusterMode::SERVER)
            return (false);
        if (clusterModule->getClusterServer() == nullptr)
            return (false);

        return (this->_clusterServerPanel.handleEvent(event, sf::Mouse::getPosition(window)));
    }

    CursorType DefaultScreen::clusterOverlayCursor()
    {
        if (this->_coreAccess == nullptr)
            return (CursorType::ARROW);

        IClusterModule *clusterModule = this->_coreAccess->getClusterModule();
        if (clusterModule == nullptr || clusterModule->getClusterMode() != ClusterMode::SERVER)
            return (CursorType::ARROW);
        if (clusterModule->getClusterServer() == nullptr)
            return (CursorType::ARROW);

        return (this->_clusterServerPanel.getCursor());
    }

    void DefaultScreen::drawMovementIndicator(sf::RenderWindow &window)
    {
        if (!this->_font)
            return;

        sf::Text label;
        label.setFont(*this->_font);
        label.setString("MOVEMENT MODE");
        label.setCharacterSize(13);
        label.setFillColor(theme::TEXT_WHITE);

        const float padX = 12.0f;
        const float dot = 8.0f;
        const float gap = 8.0f;
        const float badgeH = 26.0f;
        const float badgeW = padX + dot + gap + label.getLocalBounds().width + padX;
        const float margin = 14.0f;

        const sf::Vector2u size = window.getSize();
        const float x = this->_sidebarWidth + margin;
        const float y = static_cast<float>(size.y) - margin - badgeH;

        sf::RectangleShape badge({badgeW, badgeH});
        badge.setPosition(x, y);
        badge.setFillColor(theme::withAlpha(theme::CHECKED, 235));
        badge.setOutlineThickness(1.0f);
        badge.setOutlineColor(theme::OUTLINE);
        window.draw(badge);

        sf::CircleShape statusDot(dot / 2.0f);
        statusDot.setPosition(x + padX, y + (badgeH - dot) / 2.0f);
        statusDot.setFillColor(theme::TEXT_WHITE);
        window.draw(statusDot);

        label.setPosition(x + padX + dot + gap, y + 5.0f);
        window.draw(label);
    }

    void DefaultScreen::syncSelectionToRenderer()
    {
        if (!this->_coreAccess)
            return;
        ISceneRenderer *renderer = this->_coreAccess->getViewportRenderer();
        auto *selection_renderer = dynamic_cast<ISelectionAwareRenderer *>(renderer);
        if (selection_renderer)
        {
            selection_renderer->setSelection(this->_hierarchyPanel.getSelection());
        }
        std::vector<const Material *> materials;
        if (this->_coreAccess->getScene())
            this->_objectPanel.setScene(this->_coreAccess->getScene());
        if (this->_hierarchyPanel.tryCast<const ISceneObject>())
           this->_objectPanel.rebuild(this->_hierarchyPanel.tryCast<const ISceneObject>());
        this->syncVertexNavigator();
        if (this->_hierarchyPanel.isCameraSelected())
        {
            this->_cameraPanel.rebuild(&this->_coreAccess->getScene()->getCamera());
        }
        else
        {
            this->_cameraPanel.unselect();
        }
        const auto *primitive = this->_hierarchyPanel.tryCast<const IPrimitive>();
        if (primitive)
            this->_materialPanel.rebuild(primitive);
    }

    void DefaultScreen::updateSelectionFromClick(const sf::Vector2i &mouse)
    {
        if (this->_viewMode != ViewMode::VIEWPORT)
            return;

        IScene *scene = this->_coreAccess ? this->_coreAccess->getScene() : nullptr;
        if (!scene)
            return;

        sf::Vector2i pixel;
        if (!this->_rendererPanel.getViewportPixel(mouse, pixel))
            return;

        const ISceneObject *selectedObject = ViewportHelper::pickViewportLight(*scene, scene->getCamera(), pixel);

        if (!selectedObject)
        {
            Intersection hit;
            const Ray ray = scene->getCamera().generateRay(pixel.x, pixel.y);
            const bool has_hit = scene->intersect(ray, 0.001f, std::numeric_limits<float>::infinity(), hit);
            if (has_hit && hit.primitive)
                selectedObject = hit.primitive;
        }

        if (!selectedObject)
        {
            this->_hierarchyPanel.applyViewportSelection({});
            this->syncSelectionToRenderer();
            return;
        }

        const bool ctrl_pressed = sf::Keyboard::isKeyPressed(sf::Keyboard::LControl) || sf::Keyboard::isKeyPressed(sf::Keyboard::RControl);

        std::vector<const ISceneObject *> selection = this->_hierarchyPanel.getSelection();

        if (ctrl_pressed)
        {
            auto it = std::find(selection.begin(), selection.end(), selectedObject);
            if (it != selection.end())
                selection.erase(it);
            else
                selection.push_back(selectedObject);
        }
        else
        {
            selection.clear();
            selection.push_back(selectedObject);
        }

        this->_hierarchyPanel.applyViewportSelection(selection);
        this->syncSelectionToRenderer();
    }

    void DefaultScreen::clearHover()
    {
        if (!this->_coreAccess)
            return;
        auto *hover_renderer = dynamic_cast<ISelectionAwareRenderer *>(this->_coreAccess->getViewportRenderer());
        if (hover_renderer)
            hover_renderer->setHover(nullptr);
    }

    void DefaultScreen::updateHoverFromMouse(const sf::Vector2i &mouse)
    {
        if (!this->_coreAccess)
            return;

        auto *hover_renderer = dynamic_cast<ISelectionAwareRenderer *>(this->_coreAccess->getViewportRenderer());
        if (!hover_renderer)
            return;

        IScene *scene = this->_coreAccess->getScene();
        sf::Vector2i pixel;
        if (!scene || !this->_rendererPanel.getViewportPixel(mouse, pixel))
        {
            hover_renderer->setHover(nullptr);
            return;
        }

        const ISceneObject *hoveredObject = ViewportHelper::pickViewportLight(*scene, scene->getCamera(), pixel);

        if (!hoveredObject)
        {
            Intersection hit;
            const Ray ray = scene->getCamera().generateRay(pixel.x, pixel.y);
            const bool has_hit = scene->intersect(ray, 0.001f, std::numeric_limits<float>::infinity(), hit);
            if (has_hit && hit.primitive)
                hoveredObject = hit.primitive;
        }

        hover_renderer->setHover(hoveredObject);
    }

    const ISceneObject *DefaultScreen::pickViewportObject(const sf::Vector2i &mouse)
    {
        IScene *scene = this->_coreAccess ? this->_coreAccess->getScene() : nullptr;
        if (!scene)
            return (nullptr);

        sf::Vector2i pixel;
        if (!this->_rendererPanel.getViewportPixel(mouse, pixel))
            return (nullptr);

        const ISceneObject *object = ViewportHelper::pickViewportLight(*scene, scene->getCamera(), pixel);
        if (!object)
        {
            Intersection hit;
            const Ray ray = scene->getCamera().generateRay(pixel.x, pixel.y);
            if (scene->intersect(ray, 0.001f, std::numeric_limits<float>::infinity(), hit) && hit.primitive)
                object = hit.primitive;
        }
        return (object);
    }

    void DefaultScreen::openContextMenu(const ISceneObject *object, const sf::Vector2i &mouse)
    {
        if (!object)
            return;

        std::vector<std::pair<std::string, std::function<void()>>> entries;

        const ObjectType type = object->getObjectType();
        if (type == ObjectType::PRIMITIVE || type == ObjectType::LIGHT)
        {
            const std::string hideLabel = object->isHidden() ? "Show" : "Hide";
            entries.emplace_back(hideLabel, [this]() { this->hideSelection(); });
        }
        if (this->_hierarchyPanel.getSelection().size() >= 2)
            entries.emplace_back("Group selection", [this]() { this->groupSelection(); });
        entries.emplace_back("Delete", [this]() { this->_hierarchyPanel.deleteSelection(); });

        this->_contextMenu.openAt(static_cast<float>(mouse.x), static_cast<float>(mouse.y), entries);
    }

    void DefaultScreen::hideSelection()
    {
        IScene *scene = this->_coreAccess ? this->_coreAccess->getScene() : nullptr;
        if (!scene)
            return;

        for (const ISceneObject *selected : this->_hierarchyPanel.getSelection())
        {
            auto *object = const_cast<ISceneObject *>(selected);
            if (object)
                object->setHidden(!object->isHidden());
        }
        this->markViewportBvhDirty();
        this->syncSelectionToRenderer();
    }

    void DefaultScreen::groupSelection()
    {
        IScene *scene = this->_coreAccess ? this->_coreAccess->getScene() : nullptr;
        if (!scene)
            return;

        const std::vector<const ISceneObject *> selection = this->_hierarchyPanel.getSelection();
        if (selection.size() < 2)
            return;

        std::vector<ISceneObject *> toGroup;
        for (const ISceneObject *object : selection)
        {
            if (!object)
                continue;
            bool coveredByAncestor = false;
            for (ISceneObject *ancestor = object->getParent(); ancestor && !coveredByAncestor; ancestor = ancestor->getParent())
                for (const ISceneObject *other : selection)
                    if (other == ancestor)
                    {
                        coveredByAncestor = true;
                        break;
                    }
            if (!coveredByAncestor)
                toGroup.push_back(const_cast<ISceneObject *>(object));
        }
        if (toGroup.empty())
            return;

        ISceneObject *group = scene->addGroup();
        for (ISceneObject *object : toGroup)
            scene->reparent(object, group, -1);

        this->markViewportBvhDirty();
        this->_hierarchyPanel.applyViewportSelection({group});
        this->syncSelectionToRenderer();
        this->_toastManager.push("Group created", "Grouped " + std::to_string(toGroup.size()) + " objects into a new group.", ToastType::SUCCESS);
    }

    void DefaultScreen::handleEvent(sf::RenderWindow &window, sf::Event &event)
    {
        const sf::Vector2i mouse = sf::Mouse::getPosition(window);

        if (this->_joinClusterWindow.running)
            return;

        if (this->_exploratorWindow.running)
            return;

        if (this->_loadWindow.running)
            return;

        if (this->_marketWindow.running)
            return;

        if (this->handleClusterOverlayEvent(window, event))
            return;

        if (this->handleShortcut(event))
            return;

        // G / R / S pick the gizmo tool, T toggles World/Local (viewport only,
        // object selected). Consumes the key so it doesn't leak elsewhere.
        if (this->handleGizmoShortcut(event, mouse))
            return;

        // Right mouse over the viewport is overloaded: a drag rotates the camera
        // (independently of the component routing below), while a plain click
        // (released without dragging past the threshold) opens the object
        // context menu. A right-press on a panel must not grab the camera, and
        // is handled by the hierarchy panel via consumeContextMenuRequest below.
        if (event.type == sf::Event::MouseButtonPressed &&
            event.mouseButton.button == sf::Mouse::Right)
        {
            // Ctrl + right-click in the viewport drops a 3D marker under the
            // cursor instead of orbiting or opening the context menu; the next
            // primitive added from the menu spawns there.
            if (this->isViewportCaptured(mouse)
                && (sf::Keyboard::isKeyPressed(sf::Keyboard::LControl)
                    || sf::Keyboard::isKeyPressed(sf::Keyboard::RControl)))
            {
                this->placeMarker(mouse);
                return;
            }
            this->_rightPressInViewport = this->isViewportCaptured(mouse);
            this->_rightDragged = false;
            this->_rightPressMouse = mouse;
            if (this->_rightPressInViewport)
            {
                this->_rightMouseHeld = true;
                this->_lastMouse = sf::Mouse::getPosition(window);
            }
        }

        if (event.type == sf::Event::MouseMoved && this->_rightMouseHeld && !this->_rightDragged)
        {
            const int dx = mouse.x - this->_rightPressMouse.x;
            const int dy = mouse.y - this->_rightPressMouse.y;
            if (dx * dx + dy * dy > RIGHT_CLICK_DRAG * RIGHT_CLICK_DRAG)
                this->_rightDragged = true;
        }

        if (event.type == sf::Event::MouseButtonReleased &&
            event.mouseButton.button == sf::Mouse::Right)
        {
            const bool wasClick = this->_rightPressInViewport && !this->_rightDragged;
            this->_rightMouseHeld = false;
            this->_rightPressInViewport = false;
            if (wasClick)
            {
                const ISceneObject *object = this->pickViewportObject(mouse);
                if (object)
                {
                    if (!this->_hierarchyPanel.isSelected(object))
                    {
                        this->_hierarchyPanel.applyViewportSelection({object});
                        this->syncSelectionToRenderer();
                    }
                    this->openContextMenu(object, mouse);
                }
            }
        }

        if (event.type == sf::Event::KeyPressed && event.key.code == sf::Keyboard::M
            && !event.key.control && this->_viewMode == ViewMode::VIEWPORT
            && !this->isKeyboardCaptured())
            this->toggleMovementMode();

        this->trackFlyKeys(event, mouse);

        const bool viewportMode = this->_viewMode == ViewMode::VIEWPORT;

        if (this->_vertexDragActive)
        {
            if (event.type == sf::Event::MouseMoved)
            {
                this->applyVertexDrag(mouse);
                return;
            }
            if (event.type == sf::Event::MouseButtonReleased && event.mouseButton.button == sf::Mouse::Left)
            {
                this->endVertexDrag();
                return;
            }
        }

        if (this->_axisDragActive)
        {
            if (event.type == sf::Event::MouseMoved)
            {
                this->applyAxisDrag(mouse);
                return;
            }
            if (event.type == sf::Event::MouseButtonReleased && event.mouseButton.button == sf::Mouse::Left)
            {
                this->endAxisDrag();
                return;
            }
        }

        if (this->_rotDragActive)
        {
            if (event.type == sf::Event::MouseMoved)
            {
                this->applyRotationDrag(mouse);
                return;
            }
            if (event.type == sf::Event::MouseButtonReleased && event.mouseButton.button == sf::Mouse::Left)
            {
                this->endRotationDrag();
                return;
            }
        }

        if (this->_scaleDragActive)
        {
            if (event.type == sf::Event::MouseMoved)
            {
                this->applyScaleDrag(mouse);
                return;
            }
            if (event.type == sf::Event::MouseButtonReleased && event.mouseButton.button == sf::Mouse::Left)
            {
                this->endScaleDrag();
                return;
            }
        }

        // Planar move / uniform scale / screen rotation drags, same pattern.
        if (this->_planeDragActive)
        {
            if (event.type == sf::Event::MouseMoved)
            {
                this->applyPlaneDrag(mouse);
                return;
            }
            if (event.type == sf::Event::MouseButtonReleased && event.mouseButton.button == sf::Mouse::Left)
            {
                this->endPlaneDrag();
                return;
            }
        }

        if (this->_uscaleDragActive)
        {
            if (event.type == sf::Event::MouseMoved)
            {
                this->applyUniformScaleDrag(mouse);
                return;
            }
            if (event.type == sf::Event::MouseButtonReleased && event.mouseButton.button == sf::Mouse::Left)
            {
                this->endUniformScaleDrag();
                return;
            }
        }

        if (this->_viewRotActive)
        {
            if (event.type == sf::Event::MouseMoved)
            {
                this->applyViewRotationDrag(mouse);
                return;
            }
            if (event.type == sf::Event::MouseButtonReleased && event.mouseButton.button == sf::Mouse::Left)
            {
                this->endViewRotationDrag();
                return;
            }
        }

        // Object move: keep driving an in-progress object drag and end it on
        // release, handled before component routing (same as the vertex drag).
        if (this->_objectDragActive)
        {
            if (event.type == sf::Event::MouseMoved)
            {
                this->applyObjectDrag(mouse);
                return;
            }
            if (event.type == sf::Event::MouseButtonReleased && event.mouseButton.button == sf::Mouse::Left)
            {
                this->endObjectDrag();
                return;
            }
        }

        if (viewportMode && event.type == sf::Event::KeyPressed && !this->anyUiCapturing())
        {
            if (event.key.code == sf::Keyboard::Tab)
            {
                this->toggleEditMode();
                return;
            }
            if (event.key.code == sf::Keyboard::Escape && this->_editMode)
            {
                this->exitEditMode();
                return;
            }
        }

        std::vector<Component *> candidates = {&this->_contextMenu, &this->_menuBar, &this->_rendererPanel};

        if (viewportMode)
        {
            this->layoutSidebarResize(window);
            candidates.push_back(&this->_sidebarResize);
            this->_sidebar.collectComponents(candidates);
        }

        const Component *consumer = EventRouter::route(candidates, event, mouse);

        if (viewportMode && consumer == nullptr
            && event.type == sf::Event::KeyPressed && event.key.code == sf::Keyboard::Delete)
        {
            this->_hierarchyPanel.deleteSelection();
        }

        if (this->_hierarchyPanel.consumeSelectionChanged() || this->_objectPanel.consumeMaterialChanged())
        {
            this->syncSelectionToRenderer();
        }

        if (const ISceneObject *ctxObject = this->_hierarchyPanel.consumeContextMenuRequest())
            this->openContextMenu(ctxObject, mouse);

        if (viewportMode && consumer == nullptr &&
            event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Left)
        {
            if (mouse.x > this->_sidebarWidth && mouse.y >= static_cast<int>(MENU_HEIGHT))
            {
                if (this->_editMode)
                {
                    const int vertex = this->pickVertexHandle(mouse);
                    if (vertex >= 0)
                        this->beginVertexDrag(vertex);
                    else
                    {
                        this->_selectedVertex = -1;
                        this->syncVertexEditorField();
                    }
                }
                else if (this->beginGizmoDrag(this->singleSelectedObject(), mouse))
                {
                    // A gizmo handle (arrow / plane / ring / centre / view-ring) was
                    // grabbed for the active tool; the drag is now running and the
                    // selection is kept. Nothing else to do on this press.
                }
                else
                {
                    this->updateSelectionFromClick(mouse);

                    const IPrimitive *prim = this->_hierarchyPanel.tryCast<const IPrimitive>();
                    const ISceneObject *obj = this->_hierarchyPanel.tryCast<const ISceneObject>();
                    IEditablePrimitive *editable = prim ? dynamic_cast<IEditablePrimitive *>(const_cast<IPrimitive *>(prim)) : nullptr;
                    if (editable && obj && obj == this->_editClickObject
                        && this->_editClickClock.getElapsedTime().asMilliseconds() < 350)
                        this->enterEditMode(obj, editable);
                    this->_editClickObject = obj;
                    this->_editClickClock.restart();

                    const bool ctrl = sf::Keyboard::isKeyPressed(sf::Keyboard::LControl)
                        || sf::Keyboard::isKeyPressed(sf::Keyboard::RControl);
                    const ISceneObject *underCursor = this->pickObjectAt(mouse);
                    if (!this->_editMode && !ctrl && underCursor)
                        this->beginObjectDrag(const_cast<ISceneObject *>(underCursor), mouse);
                }
            }
        }

        if (event.type == sf::Event::MouseMoved || event.type == sf::Event::MouseLeft)
        {
            if (viewportMode && consumer == nullptr && event.type == sf::Event::MouseMoved)
            {
                if (this->_editMode)
                {
                    this->_hoverVertex = this->pickVertexHandle(mouse);
                    this->clearHover();
                }
                else
                    this->updateHoverFromMouse(mouse);
            }
            else
            {
                if (this->_editMode)
                    this->_hoverVertex = -1;
                this->clearHover();
            }
        }

        if (viewportMode && this->_coreAccess
            && (event.type == sf::Event::MouseButtonPressed
                || event.type == sf::Event::MouseButtonReleased
                || event.type == sf::Event::KeyReleased))
        {
            this->_coreAccess->historyCapture();
        }
    }

    bool DefaultScreen::isViewportCaptured(const sf::Vector2i &mouse)
    {
        if (this->_viewMode != ViewMode::VIEWPORT)
            return (false);
        if (!this->_rendererPanel.viewportBounds.contains(mouse))
            return (false);

        if (this->_menuBar.isCapturing() || this->_contextMenu.isCapturing())
            return (false);

        std::vector<Component *> components;
        this->_sidebar.collectComponents(components);
        for (Component *component : components)
            if (component && component->isCapturing())
                return (false);
        return (true);
    }

    void DefaultScreen::updateViewportCamera(sf::RenderWindow &window)
    {
        IScene *scene = this->_coreAccess->getScene();

        if (!scene)
            return;

        ICamera &camera = scene->getCamera();
        const sf::Vector2i mouse = sf::Mouse::getPosition(window);

        if (this->_movementMode && this->_rightMouseHeld)
        {
            sf::Vector2i delta = mouse - this->_lastMouse;
            this->_lastMouse = mouse;

            Vector3f rot = camera.getRotation();

            rot.z += delta.x * 0.1f;
            rot.x += delta.y * 0.1f;
            rot.y = 0.0f;

            camera.setRotation(rot);
        }

        const float dt = std::clamp(this->_frameClock.restart().asSeconds(), 0.0f, 0.1f);

        this->_keyForward = this->_keyForward && sf::Keyboard::isKeyPressed(sf::Keyboard::Z);
        this->_keyBack = this->_keyBack && sf::Keyboard::isKeyPressed(sf::Keyboard::S);
        this->_keyLeft = this->_keyLeft && sf::Keyboard::isKeyPressed(sf::Keyboard::Q);
        this->_keyRight = this->_keyRight && sf::Keyboard::isKeyPressed(sf::Keyboard::D);
        this->_keyUp = this->_keyUp && sf::Keyboard::isKeyPressed(sf::Keyboard::Space);
        this->_keyDown = this->_keyDown && sf::Keyboard::isKeyPressed(sf::Keyboard::LShift);

        const bool moving = this->_keyForward || this->_keyBack || this->_keyLeft ||
                            this->_keyRight || this->_keyUp || this->_keyDown;
        if (!moving)
            return;

        const Vector3f forward = camera.getForward();
        const Vector3f right = camera.getRight();
        const float step = this->_cameraSpeed * dt;

        Vector3f pos = camera.getPosition();
        if (this->_keyForward)
            pos = pos + forward * step;
        if (this->_keyBack)
            pos = pos - forward * step;
        if (this->_keyRight)
            pos = pos + right * step;
        if (this->_keyLeft)
            pos = pos - right * step;
        if (this->_keyUp)
            pos.z += step;
        if (this->_keyDown)
            pos.z -= step;
        camera.setPosition(pos);
    }

    void DefaultScreen::trackFlyKeys(const sf::Event &event, const sf::Vector2i &mouse)
    {
        if (event.type == sf::Event::LostFocus)
        {
            this->resetFlyKeys();
            return;
        }
        if (event.type == sf::Event::KeyReleased)
            this->setFlyKey(event.key.code, false);
        else if (event.type == sf::Event::KeyPressed && !event.key.control && this->_movementMode
                 && !this->_vertexDragActive && !this->_objectDragActive
                 && !this->_axisDragActive && !this->_rotDragActive && !this->_scaleDragActive
                 && !this->_planeDragActive && !this->_uscaleDragActive && !this->_viewRotActive &&
                 (this->_rightMouseHeld || this->isViewportCaptured(mouse)))
            this->setFlyKey(event.key.code, true);
    }

    void DefaultScreen::setFlyKey(sf::Keyboard::Key key, bool pressed)
    {
        switch (key)
        {
            case sf::Keyboard::Z: this->_keyForward = pressed; break;
            case sf::Keyboard::S: this->_keyBack = pressed; break;
            case sf::Keyboard::Q: this->_keyLeft = pressed; break;
            case sf::Keyboard::D: this->_keyRight = pressed; break;
            case sf::Keyboard::Space: this->_keyUp = pressed; break;
            case sf::Keyboard::LShift: this->_keyDown = pressed; break;
            default: break;
        }
    }

    void DefaultScreen::resetFlyKeys()
    {
        this->_keyForward = false;
        this->_keyBack = false;
        this->_keyLeft = false;
        this->_keyRight = false;
        this->_keyUp = false;
        this->_keyDown = false;
    }

    void DefaultScreen::toggleMovementMode()
    {
        this->_movementMode = !this->_movementMode;
        if (!this->_movementMode)
            this->resetFlyKeys();
        if (this->_movementMode)
            this->_toastManager.push("Movement mode on",
                "Right-drag to look, Z/Q/S/D + Space/Shift to fly.", ToastType::INFO);
        else
            this->_toastManager.push("Movement mode off",
                "Camera controls are disabled.", ToastType::INFO);
    }

    void DefaultScreen::prepareFrame()
    {
        this->_activeRenderer = nullptr;

        if (!this->_coreAccess)
            return;

        if (this->_viewMode == ViewMode::RENDERING && this->_coreAccess->getRenderer())
        {
            this->_activeRenderer = this->_coreAccess->getRenderer();
        }
        else if (this->_viewMode == ViewMode::VIEWPORT && this->_coreAccess->getViewportRenderer())
        {
            this->_activeRenderer = this->_coreAccess->getViewportRenderer();
            if (this->_viewportBvhDirty)
            {
                this->_coreAccess->getScene()->buildBvh();
                this->_activeRenderer->markSceneDirty();
                this->_viewportBvhDirty = false;
            }
            this->_activeRenderer->renderScene(*this->_coreAccess->getScene());
        }
    }

    void DefaultScreen::markViewportBvhDirty()
    {
        this->_viewportBvhDirty = true;
        this->forceViewportRetrace();
    }

    void DefaultScreen::triggerSave()
    {
        if (this->_isNewScene)
        {
            if (this->_exploratorJustClosed == true)
            {
                this->_toastManager.push("Please close the current explorer window first", "An explorer window is already open.", ToastType::ERROR);
                return;
            }

            try
            {
                this->_exploratorResult = "";
                this->_exploratorJustClosed = true;

                this->_exploratorOnClose = [&]()
                {
                    if (!this->_exploratorResult.empty())
                    {
                        try
                        {
                            this->_coreAccess->saveScene(this->_exploratorResult);
                            this->_toastManager.push("Scene saved", "Scene saved successfully.", ToastType::SUCCESS);
                        }
                        catch (const std::exception &e)
                        {
                            this->_toastManager.push("Error saving scene", e.what(), ToastType::ERROR);
                        }
                    }
                    else
                    {
                        this->_toastManager.push("No file selected", "No file was selected.", ToastType::INFO);
                    }
                };

                this->_exploratorWindow.create(*this->_font, ExploratorMode::SAVE, this->_exploratorResult);
            }
            catch(const std::exception& e)
            {
                this->_toastManager.push("Error loading scene", e.what(), ToastType::ERROR);
            }

            return;
        }

        try
        {
            this->_coreAccess->saveScene(this->_coreAccess->getCurrentScenePath());
            this->_toastManager.push("Scene saved", "Scene saved successfully.", ToastType::SUCCESS);
        }
        catch (const std::exception &e)
        {
            this->_toastManager.push("Error saving scene", e.what(), ToastType::ERROR);
        }
    }

    bool DefaultScreen::isKeyboardCaptured()
    {
        if (this->_menuBar.isCapturing() || this->_contextMenu.isCapturing())
            return (true);

        std::vector<Component *> components;
        this->_sidebar.collectComponents(components);
        for (Component *component : components)
            if (component && component->isCapturing())
                return (true);
        return (false);
    }

    bool DefaultScreen::handleShortcut(const sf::Event &event)
    {
        if (event.type != sf::Event::KeyPressed || !event.key.control)
            return (false);

        if (event.key.code == sf::Keyboard::S)
        {
            this->triggerSave();
            return (true);
        }

        if (event.key.code == sf::Keyboard::Z
            && this->_viewMode == ViewMode::VIEWPORT && !this->isKeyboardCaptured())
        {
            if (event.key.shift)
                this->redoShortcut();
            else
                this->undoShortcut();
            return (true);
        }
        return (false);
    }

    void DefaultScreen::undoShortcut()
    {
        if (!this->_coreAccess)
            return;
        if (!this->_coreAccess->historyUndo())
        {
            this->_toastManager.push("Nothing to undo", "You are at the oldest change.", ToastType::INFO);
            return;
        }
        this->onSceneRestored();
        this->_toastManager.push("Undo", "Reverted the last change.", ToastType::INFO);
    }

    void DefaultScreen::redoShortcut()
    {
        if (!this->_coreAccess)
            return;
        if (!this->_coreAccess->historyRedo())
        {
            this->_toastManager.push("Nothing to redo", "You are at the latest change.", ToastType::INFO);
            return;
        }
        this->onSceneRestored();
        this->_toastManager.push("Redo", "Reapplied the change.", ToastType::INFO);
    }

    void DefaultScreen::onSceneRestored()
    {
        IScene *scene = this->_coreAccess ? this->_coreAccess->getScene() : nullptr;

        this->_hierarchyPanel.applyViewportSelection({});
        this->_hierarchyPanel.setScene(scene);
        this->clearHover();
        this->syncSelectionToRenderer();
        this->markViewportBvhDirty();

        this->_lastHistoryScene = scene;
    }

    void DefaultScreen::applyImport()
    {
        if (!this->_loadWindow.confirmed)
        {
            this->_toastManager.push("Import cancelled", "No changes were applied.", ToastType::INFO);
            return;
        }

        IScene *scene = this->_loadWindow.scene;
        IScene *loaded = this->_loadWindow.loadedScene;

        for (auto *primitive : loaded->getPrimitives())
            scene->addPrimitive(primitive);

        ICamera &camera = scene->getCamera();
        const ICamera &loadedCamera = loaded->getCamera();

        if (this->_loadWindow.loadCameraPositionCheckbox.checked)
            camera.setPosition(loadedCamera.getPosition());
        if (this->_loadWindow.loadCameraResolutionCheckbox.checked)
            camera.setResolution(loadedCamera.getResolution());
        if (this->_loadWindow.loadCameraRotationCheckbox.checked)
            camera.setRotation(loadedCamera.getRotation());
        if (this->_loadWindow.loadCameraFovCheckbox.checked)
            camera.setFov(loadedCamera.getFov());
        if (this->_loadWindow.loadCameraSppCheckbox.checked)
            camera.setSamplesPerPixel(loadedCamera.getSamplesPerPixel());

        if (this->_loadWindow.loadLightAmbientCheckbox.checked)
            scene->setAmbientCoefficient(loaded->getAmbientCoefficient());
        if (this->_loadWindow.loadLightDiffuseCheckbox.checked)
            scene->setDiffuseCoefficient(loaded->getDiffuseCoefficient());

        this->markViewportBvhDirty();
        this->_toastManager.push("Import successful", "Scene imported successfully.", ToastType::SUCCESS);
    }

    void DefaultScreen::layoutSidebarResize(const sf::RenderWindow &window)
    {
        const sf::Vector2u size = window.getSize();
        const float maxWidth = std::max(SIDEBAR_MIN + 40.f, static_cast<float>(size.x) - VIEWPORT_MIN);

        this->_sidebarWidth = std::clamp(this->_sidebarWidth, SIDEBAR_MIN, maxWidth);
        this->_sidebarResize.setRange(SIDEBAR_MIN, maxWidth);
        this->_sidebarResize.setBounds(this->_sidebarWidth, MENU_HEIGHT, static_cast<float>(size.y) - MENU_HEIGHT);
    }

    void DefaultScreen::refreshSidebarVisibility()
    {
        this->_sidebar.setVisible(SidebarStack::HIERARCHY, true);
        this->_sidebar.setVisible(SidebarStack::CAMERA, this->_hierarchyPanel.isCameraSelected());
        this->_sidebar.setVisible(SidebarStack::OBJECT, !this->_hierarchyPanel.getSelection().empty());
        this->_sidebar.setVisible(SidebarStack::MATERIAL, this->_hierarchyPanel.tryCast<const IPrimitive>() != nullptr);
    }

    void DefaultScreen::shutdown()
    {
        this->_joinClusterWindow.destroy();
        this->_exploratorWindow.destroy();
        this->_loadWindow.destroy();
        this->_marketWindow.destroy();
    }

    void DefaultScreen::applyMarketAdditions()
    {
        IScene *scene = this->_coreAccess ? this->_coreAccess->getScene() : nullptr;
        if (!scene)
            return;

        std::vector<Material> additions;
        this->_marketWindow.drainPendingAdds(additions);
        if (additions.empty())
            return;

        for (const Material &material : additions)
        {
            Material *target = scene->createMaterial(material.name);
            if (target)
                *target = material;
        }

        this->markViewportBvhDirty();
        this->syncSelectionToRenderer();
        this->_toastManager.push("Materials added",
            std::to_string(additions.size()) + " material(s) added from the market.", ToastType::SUCCESS);
    }

    bool DefaultScreen::anyUiCapturing()
    {
        if (this->_menuBar.isCapturing())
            return (true);
        std::vector<Component *> components;
        this->_sidebar.collectComponents(components);
        for (Component *component : components)
            if (component && component->isCapturing())
                return (true);
        return (false);
    }

    IEditablePrimitive *DefaultScreen::editableFromSelection() const
    {
        HierarchyPanel &panel = const_cast<HierarchyPanel &>(this->_hierarchyPanel);
        const IPrimitive *primitive = panel.tryCast<const IPrimitive>();
        if (!primitive)
            return (nullptr);
        return (dynamic_cast<IEditablePrimitive *>(const_cast<IPrimitive *>(primitive)));
    }

    void DefaultScreen::toggleEditMode()
    {
        if (this->_editMode)
        {
            this->exitEditMode();
            return;
        }
        IEditablePrimitive *editable = this->editableFromSelection();
        if (!editable)
        {
            this->_toastManager.push("Nothing to edit", "Select a single Triangle or Mesh to edit its vertices.", ToastType::INFO);
            return;
        }
        const ISceneObject *object = const_cast<HierarchyPanel &>(this->_hierarchyPanel).tryCast<const ISceneObject>();
        this->enterEditMode(object, editable);
    }

    void DefaultScreen::enterEditMode(const ISceneObject *object, IEditablePrimitive *editable)
    {
        this->_editMode = true;
        this->_editObject = object;
        this->_editTarget = editable;
        this->_selectedVertex = -1;
        this->_hoverVertex = -1;
        this->_vertexDragActive = false;
        this->resetFlyKeys();
        this->clearHover();
        this->_objectPanel.setVertexEditor(false, {0.0f, 0.0f, 0.0f});
        this->syncVertexNavigator();
        const std::string name = object ? object->getName() : "object";
        this->_toastManager.push("Edit mode — " + name, "Drag a handle to move a vertex. X/Y/Z locks an axis. Tab/Esc to exit.", ToastType::INFO);
    }

    void DefaultScreen::exitEditMode()
    {
        const bool wasEditing = this->_editMode;
        this->_editMode = false;
        this->_editObject = nullptr;
        this->_editTarget = nullptr;
        this->_selectedVertex = -1;
        this->_hoverVertex = -1;
        this->_vertexDragActive = false;
        this->_objectPanel.setVertexEditor(false, {0.0f, 0.0f, 0.0f});
        this->syncVertexNavigator();
        if (wasEditing)
            this->_toastManager.push("Edit mode off", "Back to object mode.", ToastType::INFO);
    }

    bool DefaultScreen::vertexHandleWindowPos(std::size_t index, sf::Vector2f &out) const
    {
        if (!this->_coreAccess || !this->_editTarget)
            return (false);
        IScene *scene = this->_coreAccess->getScene();
        if (!scene)
            return (false);
        const ICamera &camera = scene->getCamera();
        const Vector2i resolution = camera.getResolution();
        sf::Vector2i pixel;
        if (!ViewportHelper::projectToPixel(camera, this->_editTarget->getVertex(index), resolution.x, resolution.y, pixel))
            return (false);
        const sf::IntRect &bounds = this->_rendererPanel.viewportBounds;
        const float scale = this->_rendererPanel.viewportScale;
        out.x = static_cast<float>(bounds.left) + static_cast<float>(pixel.x) * scale;
        out.y = static_cast<float>(bounds.top) + static_cast<float>(pixel.y) * scale;
        return (true);
    }

    int DefaultScreen::pickVertexHandle(const sf::Vector2i &mouse) const
    {
        if (!this->_editTarget || !this->_coreAccess || !this->_coreAccess->getScene())
            return (-1);
        const Vector3f cameraPos = this->_coreAccess->getScene()->getCamera().getPosition();
        const float pickRadius = 9.0f;
        const float pickRadiusSq = pickRadius * pickRadius;

        int best = -1;
        float bestDistSq = pickRadiusSq;
        float bestCameraDist = 0.0f;
        const std::size_t count = this->_editTarget->getVertexCount();
        for (std::size_t i = 0; i < count; ++i)
        {
            sf::Vector2f handle;
            if (!this->vertexHandleWindowPos(i, handle))
                continue;
            const float dx = handle.x - static_cast<float>(mouse.x);
            const float dy = handle.y - static_cast<float>(mouse.y);
            const float distSq = dx * dx + dy * dy;
            if (distSq > pickRadiusSq)
                continue;
            const float cameraDist = static_cast<float>((this->_editTarget->getVertex(i) - cameraPos).length());
            if (best == -1 || distSq < bestDistSq - 0.5f
                || (std::fabs(distSq - bestDistSq) <= 0.5f && cameraDist < bestCameraDist))
            {
                best = static_cast<int>(i);
                bestDistSq = distSq;
                bestCameraDist = cameraDist;
            }
        }
        return (best);
    }

    void DefaultScreen::beginVertexDrag(int index)
    {
        this->_selectedVertex = index;
        this->_vertexDragActive = true;
        this->_dragStartWorld = this->_editTarget->getVertex(static_cast<std::size_t>(index));
        this->resetFlyKeys();
        this->syncVertexEditorField();
    }

    void DefaultScreen::applyVertexDrag(const sf::Vector2i &mouse)
    {
        if (!this->_editTarget || this->_selectedVertex < 0 || !this->_coreAccess)
            return;
        IScene *scene = this->_coreAccess->getScene();
        if (!scene)
            return;
        const ICamera &camera = scene->getCamera();

        sf::Vector2i pixel;
        if (!this->_rendererPanel.getViewportPixel(mouse, pixel))
            return;
        const Vector2i resolution = camera.getResolution();
        const Ray ray = ViewportHelper::rayThroughPixel(camera, pixel.x, pixel.y, resolution.x, resolution.y);

        const Vector3f normal = camera.getForward();
        const float denom = dot(ray.direction, normal);
        if (std::fabs(denom) < 1e-6f)
            return;
        const float t = dot(this->_dragStartWorld - ray.origin, normal) / denom;
        if (t <= 0.0f)
            return;
        Vector3f world = ray.origin + ray.direction * t;

        // Optional world-axis lock while X, Y or Z is held.
        Vector3f axis = {0.0f, 0.0f, 0.0f};
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::X))
            axis = {1.0f, 0.0f, 0.0f};
        else if (sf::Keyboard::isKeyPressed(sf::Keyboard::Y))
            axis = {0.0f, 1.0f, 0.0f};
        else if (sf::Keyboard::isKeyPressed(sf::Keyboard::Z))
            axis = {0.0f, 0.0f, 1.0f};
        if (axis.x != 0.0f || axis.y != 0.0f || axis.z != 0.0f)
        {
            const Vector3f delta = world - this->_dragStartWorld;
            world = this->_dragStartWorld + axis * dot(delta, axis);
        }

        this->_editTarget->setVertex(static_cast<std::size_t>(this->_selectedVertex), world);
        this->markViewportBvhDirty();
        this->forceViewportRetrace();
        this->syncVertexEditorField();
    }

    void DefaultScreen::endVertexDrag()
    {
        if (!this->_vertexDragActive)
            return;
        this->_vertexDragActive = false;
        if (this->_editTarget)
            this->_editTarget->onGeometryChanged();
        this->markViewportBvhDirty();
        this->forceViewportRetrace();
        this->syncVertexEditorField();
    }

    void DefaultScreen::forceViewportRetrace()
    {
        if (!this->_coreAccess)
            return;
        auto *selection_renderer = dynamic_cast<ISelectionAwareRenderer *>(this->_coreAccess->getViewportRenderer());
        if (selection_renderer)
            selection_renderer->setSelection(this->_hierarchyPanel.getSelection());
    }

    void DefaultScreen::syncVertexEditorField()
    {
        if (this->_editMode && this->_editTarget && this->_selectedVertex >= 0)
            this->_objectPanel.setVertexEditor(true, this->_editTarget->getVertex(static_cast<std::size_t>(this->_selectedVertex)));
        else
            this->_objectPanel.setVertexEditor(false, {0.0f, 0.0f, 0.0f});
        this->syncVertexNavigator();
    }

    void DefaultScreen::convertSelectionToMesh()
    {
        if (!this->_coreAccess)
            return;
        IScene *scene = this->_coreAccess->getScene();
        if (!scene)
            return;
        const IPrimitive *primitive = this->_hierarchyPanel.tryCast<const IPrimitive>();
        if (!primitive)
            return;
        if (this->_editMode)
            this->exitEditMode();

        IPrimitive *mesh = scene->convertToMesh(const_cast<IPrimitive *>(primitive));
        if (!mesh)
        {
            this->_toastManager.push("Cannot convert", "This primitive can't be meshed (infinite, e.g. a plane, or already a mesh).", ToastType::INFO);
            return;
        }
        this->markViewportBvhDirty();
        this->forceViewportRetrace();
        this->_hierarchyPanel.setScene(scene);
        const ISceneObject *asObject = dynamic_cast<const ISceneObject *>(mesh);
        if (asObject)
            this->_hierarchyPanel.applyViewportSelection({asObject});
        this->_toastManager.push("Converted to mesh", "Now an editable mesh — press Tab to move its vertices.", ToastType::SUCCESS);
    }

    const ISceneObject *DefaultScreen::pickObjectAt(const sf::Vector2i &mouse)
    {
        IScene *scene = this->_coreAccess ? this->_coreAccess->getScene() : nullptr;
        if (!scene)
            return (nullptr);
        sf::Vector2i pixel;
        if (!this->_rendererPanel.getViewportPixel(mouse, pixel))
            return (nullptr);
        const ISceneObject *object = ViewportHelper::pickViewportLight(*scene, scene->getCamera(), pixel);
        if (!object)
        {
            Intersection hit;
            const Ray ray = scene->getCamera().generateRay(pixel.x, pixel.y);
            if (scene->intersect(ray, 0.001f, std::numeric_limits<float>::infinity(), hit) && hit.primitive)
                object = hit.primitive;
        }
        return (object);
    }

    bool DefaultScreen::viewportPlanePoint(const sf::Vector2i &mouse, const Vector3f &planeOrigin, Vector3f &out)
    {
        IScene *scene = this->_coreAccess ? this->_coreAccess->getScene() : nullptr;
        if (!scene)
            return (false);
        sf::Vector2i pixel;
        if (!this->_rendererPanel.getViewportPixel(mouse, pixel))
            return (false);
        const ICamera &camera = scene->getCamera();
        const Vector2i resolution = camera.getResolution();
        const Ray ray = ViewportHelper::rayThroughPixel(camera, pixel.x, pixel.y, resolution.x, resolution.y);
        const Vector3f normal = camera.getForward();
        const float denom = dot(ray.direction, normal);
        if (std::fabs(denom) < 1e-6f)
            return (false);
        const float t = dot(planeOrigin - ray.origin, normal) / denom;
        if (t <= 0.0f)
            return (false);
        out = ray.origin + ray.direction * t;
        return (true);
    }

    void DefaultScreen::beginObjectDrag(ISceneObject *object, const sf::Vector2i &mouse)
    {
        if (!object)
            return;
        this->_objectDragActive = true;
        this->_objectDragMoved = false;
        this->_objectDragTarget = object;
        const Vector3f objPos = object->getLocalPosition();
        this->_objectDragPlaneOrigin = objPos;
        Vector3f grab;
        if (this->viewportPlanePoint(mouse, objPos, grab))
            this->_objectDragOffset = objPos - grab;
        else
            this->_objectDragOffset = {0.0f, 0.0f, 0.0f};
    }

    void DefaultScreen::applyObjectDrag(const sf::Vector2i &mouse)
    {
        if (!this->_objectDragTarget)
            return;
        Vector3f grab;
        if (!this->viewportPlanePoint(mouse, this->_objectDragPlaneOrigin, grab))
            return;
        Vector3f newPos = grab + this->_objectDragOffset;

        Vector3f axis = {0.0f, 0.0f, 0.0f};
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::X))
            axis = {1.0f, 0.0f, 0.0f};
        else if (sf::Keyboard::isKeyPressed(sf::Keyboard::Y))
            axis = {0.0f, 1.0f, 0.0f};
        else if (sf::Keyboard::isKeyPressed(sf::Keyboard::Z))
            axis = {0.0f, 0.0f, 1.0f};
        if (axis.x != 0.0f || axis.y != 0.0f || axis.z != 0.0f)
        {
            const Vector3f delta = newPos - this->_objectDragPlaneOrigin;
            newPos = this->_objectDragPlaneOrigin + axis * dot(delta, axis);
        }

        this->_objectDragTarget->setLocalPosition(newPos);
        this->_objectDragMoved = true;
        this->markViewportBvhDirty();
        this->forceViewportRetrace();
    }

    void DefaultScreen::endObjectDrag()
    {
        if (!this->_objectDragActive)
            return;
        this->_objectDragActive = false;
        if (this->_objectDragMoved && this->_objectDragTarget)
        {
            this->_objectPanel.rebuild(this->_objectDragTarget);
            this->markViewportBvhDirty();
            this->forceViewportRetrace();
        }
        this->_objectDragTarget = nullptr;
    }

    namespace
    {
        Vector3f axisVector(int axis)
        {
            if (axis == 0)
                return {1.0f, 0.0f, 0.0f};
            if (axis == 1)
                return {0.0f, 1.0f, 0.0f};
            return {0.0f, 0.0f, 1.0f};
        }

        // Shortest distance from point p to the segment [a, b], in 2D.
        float segmentDistance(const sf::Vector2f &p, const sf::Vector2f &a, const sf::Vector2f &b)
        {
            const sf::Vector2f ab(b.x - a.x, b.y - a.y);
            const sf::Vector2f ap(p.x - a.x, p.y - a.y);
            const float len2 = ab.x * ab.x + ab.y * ab.y;
            float t = (len2 > 1e-6f) ? (ap.x * ab.x + ap.y * ab.y) / len2 : 0.0f;
            t = std::max(0.0f, std::min(1.0f, t));
            const sf::Vector2f proj(a.x + ab.x * t, a.y + ab.y * t);
            const sf::Vector2f d(p.x - proj.x, p.y - proj.y);
            return std::sqrt(d.x * d.x + d.y * d.y);
        }

        constexpr float SNAP_MOVE = 1.0f;    // world units
        constexpr float SNAP_ROT = 15.0f;    // degrees
        constexpr float SNAP_SCALE = 0.1f;   // scale factor / value

        bool snapHeld()
        {
            return (sf::Keyboard::isKeyPressed(sf::Keyboard::LControl)
                || sf::Keyboard::isKeyPressed(sf::Keyboard::RControl));
        }

        float snapTo(float value, float step)
        {
            return (step > 0.0f) ? std::round(value / step) * step : value;
        }

        std::string fmtNum(float v)
        {
            char buf[32];
            std::snprintf(buf, sizeof(buf), "%.2f", v);
            return (std::string(buf));
        }

        // Rotate the Euler triple `startDeg` (applied as Rz*Ry*Rx) by `angleDeg`
        // about the world unit axis `axis`, and return the resulting Euler triple
        // in degrees (same ZYX convention as rotate()).
        Vector3f eulerCompose(const Vector3f &startDeg, const Vector3f &axis, float angleDeg)
        {
            const Vector3f r = degToRad(startDeg);
            const float cx = std::cos(r.x), sx = std::sin(r.x);
            const float cy = std::cos(r.y), sy = std::sin(r.y);
            const float cz = std::cos(r.z), sz = std::sin(r.z);
            const float R0[3][3] = {
                {cz * cy, cz * sy * sx - sz * cx, cz * sy * cx + sz * sx},
                {sz * cy, sz * sy * sx + cz * cx, sz * sy * cx - cz * sx},
                {-sy, cy * sx, cy * cx},
            };
            const float len = std::sqrt(dot(axis, axis));
            if (len < 1e-6f)
                return (startDeg);
            const float ax = axis.x / len, ay = axis.y / len, az = axis.z / len;
            const float a = angleDeg * 3.14159265358979323846f / 180.0f;
            const float c = std::cos(a), s = std::sin(a), t = 1.0f - c;
            const float Rd[3][3] = {
                {t * ax * ax + c, t * ax * ay - s * az, t * ax * az + s * ay},
                {t * ax * ay + s * az, t * ay * ay + c, t * ay * az - s * ax},
                {t * ax * az - s * ay, t * ay * az + s * ax, t * az * az + c},
            };
            float R[3][3];
            for (int i = 0; i < 3; ++i)
                for (int j = 0; j < 3; ++j)
                    R[i][j] = Rd[i][0] * R0[0][j] + Rd[i][1] * R0[1][j] + Rd[i][2] * R0[2][j];
            const float deg = 180.0f / 3.14159265358979323846f;
            const float ry = std::atan2(-R[2][0], std::sqrt(R[0][0] * R[0][0] + R[1][0] * R[1][0]));
            const float rx = std::atan2(R[2][1], R[2][2]);
            const float rz = std::atan2(R[1][0], R[0][0]);
            return {rx * deg, ry * deg, rz * deg};
        }
    }

    ISceneObject *DefaultScreen::singleSelectedObject() const
    {
        const std::vector<const ISceneObject *> &selection = this->_hierarchyPanel.getSelection();
        if (selection.size() != 1 || !selection[0])
            return (nullptr);
        return (const_cast<ISceneObject *>(selection[0]));
    }

    bool DefaultScreen::projectToViewport(const Vector3f &point, sf::Vector2f &out) const
    {
        IScene *scene = this->_coreAccess ? this->_coreAccess->getScene() : nullptr;
        if (!scene)
            return (false);
        const ICamera &camera = scene->getCamera();
        const Vector2i resolution = camera.getResolution();
        if (resolution.x <= 0 || resolution.y <= 0)
            return (false);

        const Vector3f forward = camera.getForward();
        const Vector3f right = camera.getRight();
        const Vector3f up = right.cross(forward).unit_vector();
        const Vector3f toPoint = point - camera.getPosition();
        const float z = dot(toPoint, forward);
        if (z <= 0.001f) // behind the camera
            return (false);

        constexpr float PI = 3.14159265358979323846f;
        const float theta = static_cast<float>(camera.getFov()) * (PI / 180.0f);
        const float viewportHeight = 2.0f * std::tan(theta / 2.0f);
        const float viewportWidth = viewportHeight
            * (static_cast<float>(resolution.x) / static_cast<float>(resolution.y));
        const float ndcX = (dot(toPoint, right) / z) / (viewportWidth / 2.0f);
        const float ndcY = (dot(toPoint, up) / z) / (viewportHeight / 2.0f);
        const float px = (ndcX * 0.5f + 0.5f) * static_cast<float>(resolution.x - 1);
        const float py = (-ndcY * 0.5f + 0.5f) * static_cast<float>(resolution.y - 1);

        const sf::IntRect &bounds = this->_rendererPanel.viewportBounds;
        const float scale = this->_rendererPanel.viewportScale;
        out.x = static_cast<float>(bounds.left) + px * scale;
        out.y = static_cast<float>(bounds.top) + py * scale;
        return (true);
    }

    bool DefaultScreen::gizmoArrow(int axis, sf::Vector2f &origin, sf::Vector2f &tip) const
    {
        ISceneObject *object = this->singleSelectedObject();
        if (!object || !this->_coreAccess)
            return (false);
        IScene *scene = this->_coreAccess->getScene();
        if (!scene)
            return (false);
        const Vector3f objPos = object->getPosition();
        const Vector3f toCam = objPos - scene->getCamera().getPosition();
        float length = std::sqrt(dot(toCam, toCam)) * 0.16f;
        if (length < 1e-4f)
            length = 1.0f;
        if (!this->projectToViewport(objPos, origin))
            return (false);
        if (!this->projectToViewport(objPos + this->gizmoAxisDir(axis) * length, tip))
            return (false);
        return (true);
    }

    int DefaultScreen::pickGizmoAxis(const sf::Vector2i &mouse) const
    {
        if (this->_editMode || !this->singleSelectedObject())
            return (-1);
        if (!this->_rendererPanel.viewportBounds.contains(mouse))
            return (-1);
        const sf::Vector2f m(static_cast<float>(mouse.x), static_cast<float>(mouse.y));
        int best = -1;
        float bestDistance = 8.0f; // pick radius in pixels
        for (int axis = 0; axis < 3; ++axis)
        {
            sf::Vector2f origin;
            sf::Vector2f tip;
            if (!this->gizmoArrow(axis, origin, tip))
                continue;
            const float d = segmentDistance(m, origin, tip);
            if (d < bestDistance)
            {
                bestDistance = d;
                best = axis;
            }
        }
        return (best);
    }

    void DefaultScreen::drawMoveGizmo(sf::RenderWindow &window) const
    {
        if (this->_editMode || !this->singleSelectedObject())
            return;
        constexpr float PI = 3.14159265358979323846f;
        const sf::Color colors[3] = {sf::Color(235, 80, 80), sf::Color(95, 205, 100), sf::Color(95, 160, 245)};
        bool centerDrawn = false;
        sf::Vector2f center;
        for (int axis = 0; axis < 3; ++axis)
        {
            sf::Vector2f origin;
            sf::Vector2f tip;
            if (!this->gizmoArrow(axis, origin, tip))
                continue;
            center = origin;
            centerDrawn = true;
            const sf::Vector2f d(tip.x - origin.x, tip.y - origin.y);
            const float len = std::sqrt(d.x * d.x + d.y * d.y);
            if (len < 1.0f)
                continue;
            const float angle = std::atan2(d.y, d.x) * 180.0f / PI;

            const bool active = this->_axisDragActive && this->_axisDragAxis == axis;
            const sf::Color color = active ? sf::Color(255, 235, 90) : colors[axis];

            sf::RectangleShape shaft({len - 6.0f, active ? 4.0f : 3.0f});
            shaft.setOrigin(0.0f, (active ? 4.0f : 3.0f) / 2.0f);
            shaft.setPosition(origin);
            shaft.setRotation(angle);
            shaft.setFillColor(color);
            window.draw(shaft);

            sf::CircleShape head(6.5f, 3);
            head.setOrigin(6.5f, 6.5f);
            head.setPosition(tip);
            head.setRotation(angle + 90.0f);
            head.setFillColor(color);
            window.draw(head);
        }
        if (centerDrawn)
        {
            sf::CircleShape hub(3.5f);
            hub.setOrigin(3.5f, 3.5f);
            hub.setPosition(center);
            hub.setFillColor(sf::Color(240, 240, 240));
            window.draw(hub);
        }
    }

    bool DefaultScreen::axisParamFromMouse(const sf::Vector2i &mouse, const Vector3f &axisOrigin,
        const Vector3f &axisDir, float &t) const
    {
        IScene *scene = this->_coreAccess ? this->_coreAccess->getScene() : nullptr;
        if (!scene)
            return (false);
        sf::Vector2i pixel;
        if (!this->_rendererPanel.getViewportPixel(mouse, pixel))
            return (false);
        const ICamera &camera = scene->getCamera();
        const Vector2i resolution = camera.getResolution();
        const Ray ray = ViewportHelper::rayThroughPixel(camera, pixel.x, pixel.y, resolution.x, resolution.y);
        const Vector3f o = ray.origin;
        const Vector3f r = ray.direction.unit_vector();

        // Closest point on the axis line (axisOrigin + t*axisDir, axisDir unit) to
        // the mouse ray (o + s*r, r unit): with a = c = 1, t = (b*e - d) / (1 - b^2).
        const Vector3f w0 = axisOrigin - o;
        const float b = dot(axisDir, r);
        const float d = dot(axisDir, w0);
        const float e = dot(r, w0);
        const float denom = 1.0f - b * b;
        if (std::fabs(denom) < 1e-6f) // ray nearly parallel to the axis: ill-defined
            return (false);
        t = (b * e - d) / denom;
        return (true);
    }

    void DefaultScreen::beginAxisDrag(ISceneObject *object, int axis, const sf::Vector2i &mouse)
    {
        if (!object)
            return;
        this->_axisDragActive = true;
        this->_axisDragMoved = false;
        this->_axisDragTarget = object;
        this->_axisDragAxis = axis;
        this->_axisDragObjStart = object->getPosition();
        this->_axisDragDir = this->gizmoAxisDir(axis);
        float t = 0.0f;
        this->_axisDragGrabT =
            this->axisParamFromMouse(mouse, this->_axisDragObjStart, this->_axisDragDir, t) ? t : 0.0f;
    }

    void DefaultScreen::applyAxisDrag(const sf::Vector2i &mouse)
    {
        if (!this->_axisDragTarget)
            return;
        float t = 0.0f;
        if (!this->axisParamFromMouse(mouse, this->_axisDragObjStart, this->_axisDragDir, t))
            return;
        float delta = t - this->_axisDragGrabT;
        if (snapHeld())
            delta = snapTo(delta, SNAP_MOVE);
        const Vector3f newPos = this->_axisDragObjStart + this->_axisDragDir * delta;
        this->_axisDragTarget->setLocalPosition(newPos);
        this->_axisDragMoved = true;
        const char axisName[3] = {'X', 'Y', 'Z'};
        this->_gizmoReadout = std::string(1, axisName[this->_axisDragAxis]) + " " + fmtNum(delta);
        this->markViewportBvhDirty();
        this->forceViewportRetrace();
    }

    void DefaultScreen::endAxisDrag()
    {
        if (!this->_axisDragActive)
            return;
        this->_axisDragActive = false;
        if (this->_axisDragMoved && this->_axisDragTarget)
        {
            this->_objectPanel.rebuild(this->_axisDragTarget);
            this->markViewportBvhDirty();
            this->forceViewportRetrace();
        }
        this->_axisDragMoved = false;
        this->_axisDragTarget = nullptr;
        this->_axisDragAxis = -1;
        this->_gizmoReadout.clear();
    }

    namespace
    {
        constexpr int RING_SEGMENTS = 48;
    }

    bool DefaultScreen::rayPlanePoint(const sf::Vector2i &mouse, const Vector3f &origin,
        const Vector3f &normal, Vector3f &out) const
    {
        IScene *scene = this->_coreAccess ? this->_coreAccess->getScene() : nullptr;
        if (!scene)
            return (false);
        sf::Vector2i pixel;
        if (!this->_rendererPanel.getViewportPixel(mouse, pixel))
            return (false);
        const ICamera &camera = scene->getCamera();
        const Vector2i resolution = camera.getResolution();
        const Ray ray = ViewportHelper::rayThroughPixel(camera, pixel.x, pixel.y, resolution.x, resolution.y);
        const float denom = dot(ray.direction, normal);
        if (std::fabs(denom) < 1e-6f)
            return (false);
        const float t = dot(origin - ray.origin, normal) / denom;
        if (t <= 0.0f)
            return (false);
        out = ray.origin + ray.direction * t;
        return (true);
    }

    bool DefaultScreen::rotationRing(int axis, std::vector<sf::Vector2f> &pts, std::vector<char> &valid) const
    {
        ISceneObject *object = this->singleSelectedObject();
        if (!object || !this->_coreAccess)
            return (false);
        IScene *scene = this->_coreAccess->getScene();
        if (!scene)
            return (false);
        const Vector3f objPos = object->getPosition();
        const Vector3f toCam = objPos - scene->getCamera().getPosition();
        float radius = std::sqrt(dot(toCam, toCam)) * 0.19f; // just outside the move arrows
        if (radius < 1e-4f)
            radius = 1.2f;
        // The ring for `axis` lies in the plane spanned by the other two axes
        // (world axes, or the object's local axes when Local space is on).
        const Vector3f u = this->gizmoAxisDir((axis + 1) % 3);
        const Vector3f v = this->gizmoAxisDir((axis + 2) % 3);

        constexpr float PI = 3.14159265358979323846f;
        pts.assign(RING_SEGMENTS + 1, {0.0f, 0.0f});
        valid.assign(RING_SEGMENTS + 1, 0);
        for (int i = 0; i <= RING_SEGMENTS; ++i)
        {
            const float theta = 2.0f * PI * static_cast<float>(i) / static_cast<float>(RING_SEGMENTS);
            const Vector3f world = objPos + (u * std::cos(theta) + v * std::sin(theta)) * radius;
            sf::Vector2f screen;
            valid[i] = this->projectToViewport(world, screen) ? 1 : 0;
            pts[i] = screen;
        }
        return (true);
    }

    int DefaultScreen::pickRotationRing(const sf::Vector2i &mouse) const
    {
        if (this->_editMode || !this->singleSelectedObject())
            return (-1);
        if (!this->_rendererPanel.viewportBounds.contains(mouse))
            return (-1);
        const sf::Vector2f m(static_cast<float>(mouse.x), static_cast<float>(mouse.y));
        int best = -1;
        float bestDistance = 7.0f; // pick radius in pixels
        std::vector<sf::Vector2f> pts;
        std::vector<char> valid;
        for (int axis = 0; axis < 3; ++axis)
        {
            if (!this->rotationRing(axis, pts, valid))
                continue;
            for (int i = 0; i < RING_SEGMENTS; ++i)
            {
                if (!valid[i] || !valid[i + 1])
                    continue;
                const float d = segmentDistance(m, pts[i], pts[i + 1]);
                if (d < bestDistance)
                {
                    bestDistance = d;
                    best = axis;
                }
            }
        }
        return (best);
    }

    void DefaultScreen::drawRotationRings(sf::RenderWindow &window) const
    {
        if (this->_editMode || !this->singleSelectedObject())
            return;
        constexpr float PI = 3.14159265358979323846f;
        const sf::Color colors[3] = {sf::Color(235, 80, 80), sf::Color(95, 205, 100), sf::Color(95, 160, 245)};
        std::vector<sf::Vector2f> pts;
        std::vector<char> valid;
        for (int axis = 0; axis < 3; ++axis)
        {
            if (!this->rotationRing(axis, pts, valid))
                continue;
            const bool active = this->_rotDragActive && this->_rotDragAxis == axis;
            const sf::Color color = active ? sf::Color(255, 235, 90) : colors[axis];
            const float thick = active ? 3.0f : 2.0f;
            for (int i = 0; i < RING_SEGMENTS; ++i)
            {
                if (!valid[i] || !valid[i + 1])
                    continue;
                const sf::Vector2f a = pts[i];
                const sf::Vector2f b = pts[i + 1];
                const sf::Vector2f d(b.x - a.x, b.y - a.y);
                const float len = std::sqrt(d.x * d.x + d.y * d.y);
                if (len < 0.5f)
                    continue;
                sf::RectangleShape seg({len, thick});
                seg.setOrigin(0.0f, thick / 2.0f);
                seg.setPosition(a);
                seg.setRotation(std::atan2(d.y, d.x) * 180.0f / PI);
                seg.setFillColor(color);
                window.draw(seg);
            }
        }
    }

    void DefaultScreen::beginRotationDrag(ISceneObject *object, int axis, const sf::Vector2i &mouse)
    {
        if (!object)
            return;
        this->_rotDragActive = true;
        this->_rotDragMoved = false;
        this->_rotDragTarget = object;
        this->_rotDragAxis = axis;
        this->_rotDragStartRot = object->getLocalRotation();
        this->_rotDragObjPos = object->getPosition();
        this->_rotDragAxisN = this->gizmoAxisDir(axis);
        Vector3f grab;
        this->_rotDragValid = false;
        if (this->rayPlanePoint(mouse, this->_rotDragObjPos, this->_rotDragAxisN, grab))
        {
            const Vector3f gv = grab - this->_rotDragObjPos;
            const float len = std::sqrt(dot(gv, gv));
            if (len > 1e-5f)
            {
                this->_rotDragGrabVec = gv * (1.0f / len);
                this->_rotDragValid = true;
            }
        }
    }

    void DefaultScreen::applyRotationDrag(const sf::Vector2i &mouse)
    {
        if (!this->_rotDragTarget || !this->_rotDragValid)
            return;
        Vector3f point;
        if (!this->rayPlanePoint(mouse, this->_rotDragObjPos, this->_rotDragAxisN, point))
            return;
        const Vector3f cv = point - this->_rotDragObjPos;
        const float len = std::sqrt(dot(cv, cv));
        if (len < 1e-5f)
            return;
        const Vector3f cur = cv * (1.0f / len);

        // Signed angle from the grab vector to the current vector, about the axis.
        constexpr float PI = 3.14159265358979323846f;
        const float sine = dot(this->_rotDragGrabVec.cross(cur), this->_rotDragAxisN);
        const float cosine = dot(this->_rotDragGrabVec, cur);
        float angleDeg = std::atan2(sine, cosine) * 180.0f / PI;
        if (snapHeld())
            angleDeg = snapTo(angleDeg, SNAP_ROT);

        // In Local space the ring normal is the rotated axis, so compose the
        // rotation properly; in World space it reduces to bumping one Euler angle.
        Vector3f rotation;
        if (this->_gizmoLocal)
            rotation = eulerCompose(this->_rotDragStartRot, this->_rotDragAxisN, angleDeg);
        else
        {
            rotation = this->_rotDragStartRot;
            if (this->_rotDragAxis == 0)
                rotation.x += angleDeg;
            else if (this->_rotDragAxis == 1)
                rotation.y += angleDeg;
            else
                rotation.z += angleDeg;
        }
        this->_rotDragTarget->setLocalRotation(rotation);
        this->_rotDragMoved = true;
        const char axisName[3] = {'X', 'Y', 'Z'};
        this->_gizmoReadout = std::string(1, axisName[this->_rotDragAxis]) + " " + fmtNum(angleDeg) + " deg";
        this->markViewportBvhDirty();
        this->forceViewportRetrace();
    }

    void DefaultScreen::endRotationDrag()
    {
        if (!this->_rotDragActive)
            return;
        this->_rotDragActive = false;
        if (this->_rotDragMoved && this->_rotDragTarget)
        {
            this->_objectPanel.rebuild(this->_rotDragTarget);
            this->markViewportBvhDirty();
            this->forceViewportRetrace();
        }
        this->_rotDragMoved = false;
        this->_rotDragValid = false;
        this->_rotDragTarget = nullptr;
        this->_rotDragAxis = -1;
        this->_gizmoReadout.clear();
    }

    void DefaultScreen::drawScaleGizmo(sf::RenderWindow &window) const
    {
        if (this->_editMode || !this->singleSelectedObject())
            return;
        constexpr float PI = 3.14159265358979323846f;
        const sf::Color colors[3] = {sf::Color(235, 80, 80), sf::Color(95, 205, 100), sf::Color(95, 160, 245)};
        bool centerDrawn = false;
        sf::Vector2f center;
        for (int axis = 0; axis < 3; ++axis)
        {
            sf::Vector2f origin;
            sf::Vector2f tip;
            if (!this->gizmoArrow(axis, origin, tip))
                continue;
            center = origin;
            centerDrawn = true;
            const sf::Vector2f d(tip.x - origin.x, tip.y - origin.y);
            const float len = std::sqrt(d.x * d.x + d.y * d.y);
            if (len < 1.0f)
                continue;
            const float angle = std::atan2(d.y, d.x) * 180.0f / PI;
            const bool active = this->_scaleDragActive && this->_scaleDragAxis == axis;
            const sf::Color color = active ? sf::Color(255, 235, 90) : colors[axis];

            sf::RectangleShape shaft({len - 6.0f, active ? 4.0f : 3.0f});
            shaft.setOrigin(0.0f, (active ? 4.0f : 3.0f) / 2.0f);
            shaft.setPosition(origin);
            shaft.setRotation(angle);
            shaft.setFillColor(color);
            window.draw(shaft);

            sf::RectangleShape box({9.0f, 9.0f});
            box.setOrigin(4.5f, 4.5f);
            box.setPosition(tip);
            box.setRotation(angle);
            box.setFillColor(color);
            window.draw(box);
        }
        if (centerDrawn)
        {
            sf::CircleShape hub(3.5f);
            hub.setOrigin(3.5f, 3.5f);
            hub.setPosition(center);
            hub.setFillColor(sf::Color(240, 240, 240));
            window.draw(hub);
        }
    }

    void DefaultScreen::beginScaleDrag(ISceneObject *object, int axis, const sf::Vector2i &mouse)
    {
        if (!object)
            return;
        this->_scaleDragActive = true;
        this->_scaleDragMoved = false;
        this->_scaleDragTarget = object;
        this->_scaleDragAxis = axis;
        this->_scaleDragStartScale = object->getLocalScale();
        this->_scaleDragObjStart = object->getPosition();
        this->_scaleDragDir = this->gizmoAxisDir(axis);
        float t = 0.0f;
        this->_scaleDragGrabT =
            this->axisParamFromMouse(mouse, this->_scaleDragObjStart, this->_scaleDragDir, t) ? t : 0.0f;
    }

    void DefaultScreen::applyScaleDrag(const sf::Vector2i &mouse)
    {
        if (!this->_scaleDragTarget)
            return;
        if (std::fabs(this->_scaleDragGrabT) < 1e-4f) // grabbed at the centre: ill-defined
            return;
        float t = 0.0f;
        if (!this->axisParamFromMouse(mouse, this->_scaleDragObjStart, this->_scaleDragDir, t))
            return;
        float factor = t / this->_scaleDragGrabT;
        if (snapHeld())
            factor = std::max(SNAP_SCALE, snapTo(factor, SNAP_SCALE));
        Vector3f scale = this->_scaleDragStartScale;
        if (this->_scaleDragAxis == 0)
            scale.x = std::max(0.001f, this->_scaleDragStartScale.x * factor);
        else if (this->_scaleDragAxis == 1)
            scale.y = std::max(0.001f, this->_scaleDragStartScale.y * factor);
        else
            scale.z = std::max(0.001f, this->_scaleDragStartScale.z * factor);
        this->_scaleDragTarget->setLocalScale(scale);
        this->_scaleDragMoved = true;
        const char axisName[3] = {'X', 'Y', 'Z'};
        this->_gizmoReadout = std::string(1, axisName[this->_scaleDragAxis]) + " " + fmtNum(factor) + "x";
        this->markViewportBvhDirty();
        this->forceViewportRetrace();
    }

    void DefaultScreen::endScaleDrag()
    {
        if (!this->_scaleDragActive)
            return;
        this->_scaleDragActive = false;
        if (this->_scaleDragMoved && this->_scaleDragTarget)
        {
            this->_objectPanel.rebuild(this->_scaleDragTarget);
            this->markViewportBvhDirty();
            this->forceViewportRetrace();
        }
        this->_scaleDragMoved = false;
        this->_scaleDragTarget = nullptr;
        this->_scaleDragAxis = -1;
        this->_gizmoReadout.clear();
    }

    bool DefaultScreen::beginGizmoDrag(ISceneObject *object, const sf::Vector2i &mouse)
    {
        if (!object)
            return (false);
        if (this->_gizmoMode == GizmoMode::MOVE)
        {
            const int axis = this->pickGizmoAxis(mouse);
            if (axis >= 0) { this->beginAxisDrag(object, axis, mouse); return (true); }
            const int plane = this->pickPlaneHandle(mouse);
            if (plane >= 0) { this->beginPlaneDrag(object, plane, mouse); return (true); }
        }
        else if (this->_gizmoMode == GizmoMode::ROTATE)
        {
            const int axis = this->pickRotationRing(mouse);
            if (axis >= 0) { this->beginRotationDrag(object, axis, mouse); return (true); }
            if (this->pickViewRotation(mouse)) { this->beginViewRotationDrag(object, mouse); return (true); }
        }
        else if (this->_gizmoMode == GizmoMode::SCALE)
        {
            if (this->pickUniformScale(mouse)) { this->beginUniformScaleDrag(object, mouse); return (true); }
            const int axis = this->pickGizmoAxis(mouse);
            if (axis >= 0) { this->beginScaleDrag(object, axis, mouse); return (true); }
        }
        return (false);
    }

    Vector3f DefaultScreen::gizmoAxisDir(int axis) const
    {
        const Vector3f world = axisVector(axis);
        if (!this->_gizmoLocal)
            return (world);
        ISceneObject *object = this->singleSelectedObject();
        if (!object)
            return (world);
        return (rotate(world, degToRad(object->getRotation())));
    }

    void DefaultScreen::setGizmoTool(GizmoMode mode)
    {
        this->_gizmoMode = mode;
        this->_objectPanel.setGizmoMode(static_cast<int>(mode));
        this->_gizmoReadout.clear();
    }

    bool DefaultScreen::handleGizmoShortcut(const sf::Event &event, const sf::Vector2i &mouse)
    {
        if (event.type != sf::Event::KeyPressed || event.key.control)
            return (false);
        // Only when the viewport owns input, an object is selected, we are not in
        // vertex-edit mode, and movement mode is off (so G/R/S don't fight the
        // ZQSD fly keys).
        if (this->_editMode || this->_movementMode || this->anyUiCapturing())
            return (false);
        if (!this->isViewportCaptured(mouse) || !this->singleSelectedObject())
            return (false);
        switch (event.key.code)
        {
            case sf::Keyboard::G: this->setGizmoTool(GizmoMode::MOVE); return (true);
            case sf::Keyboard::R: this->setGizmoTool(GizmoMode::ROTATE); return (true);
            case sf::Keyboard::S: this->setGizmoTool(GizmoMode::SCALE); return (true);
            case sf::Keyboard::T:
                this->_gizmoLocal = !this->_gizmoLocal;
                this->_objectPanel.setGizmoSpace(this->_gizmoLocal);
                return (true);
            default: return (false);
        }
    }

    void DefaultScreen::drawGizmoReadout(sf::RenderWindow &window) const
    {
        if (this->_gizmoReadout.empty() || !this->_font || !this->singleSelectedObject())
            return;
        sf::Vector2f anchor;
        if (!this->projectToViewport(this->singleSelectedObject()->getPosition(), anchor))
            return;
        sf::Text text(this->_gizmoReadout, *this->_font, 13);
        text.setFillColor(sf::Color::White);
        const sf::FloatRect b = text.getLocalBounds();
        const sf::Vector2f pos(anchor.x + 14.0f, anchor.y - 26.0f);
        sf::RectangleShape bg({b.width + 12.0f, b.height + 10.0f});
        bg.setPosition(pos.x - 6.0f, pos.y - 5.0f);
        bg.setFillColor(sf::Color(20, 22, 28, 200));
        bg.setOutlineThickness(1.0f);
        bg.setOutlineColor(sf::Color(90, 90, 100, 200));
        text.setPosition(pos.x - b.left, pos.y - b.top);
        window.draw(bg);
        window.draw(text);
    }

    bool DefaultScreen::gizmoPlaneHandle(int plane, sf::Vector2f &out) const
    {
        ISceneObject *object = this->singleSelectedObject();
        if (!object || !this->_coreAccess)
            return (false);
        IScene *scene = this->_coreAccess->getScene();
        if (!scene)
            return (false);
        const Vector3f objPos = object->getPosition();
        const Vector3f toCam = objPos - scene->getCamera().getPosition();
        float length = std::sqrt(dot(toCam, toCam)) * 0.16f;
        if (length < 1e-4f)
            length = 1.0f;
        const Vector3f a = this->gizmoAxisDir(plane);
        const Vector3f b = this->gizmoAxisDir((plane + 1) % 3);
        return (this->projectToViewport(objPos + (a + b) * (length * 0.42f), out));
    }

    int DefaultScreen::pickPlaneHandle(const sf::Vector2i &mouse) const
    {
        if (this->_editMode || !this->singleSelectedObject())
            return (-1);
        if (!this->_rendererPanel.viewportBounds.contains(mouse))
            return (-1);
        const sf::Vector2f m(static_cast<float>(mouse.x), static_cast<float>(mouse.y));
        for (int plane = 0; plane < 3; ++plane)
        {
            sf::Vector2f handle;
            if (!this->gizmoPlaneHandle(plane, handle))
                continue;
            const float dx = m.x - handle.x;
            const float dy = m.y - handle.y;
            if (dx * dx + dy * dy <= 9.0f * 9.0f)
                return (plane);
        }
        return (-1);
    }

    void DefaultScreen::drawPlaneHandles(sf::RenderWindow &window) const
    {
        if (this->_editMode || !this->singleSelectedObject())
            return;
        // Blended colour of the two in-plane axes: XY=yellow, YZ=cyan, ZX=magenta.
        const sf::Color colors[3] = {sf::Color(230, 220, 90), sf::Color(90, 215, 215), sf::Color(215, 100, 215)};
        for (int plane = 0; plane < 3; ++plane)
        {
            sf::Vector2f handle;
            if (!this->gizmoPlaneHandle(plane, handle))
                continue;
            const bool active = this->_planeDragActive && this->_planeDragPlane == plane;
            sf::Color fill = active ? sf::Color(255, 235, 90) : colors[plane];
            sf::RectangleShape square({11.0f, 11.0f});
            square.setOrigin(5.5f, 5.5f);
            square.setPosition(handle);
            sf::Color body = fill;
            body.a = 130;
            square.setFillColor(body);
            square.setOutlineThickness(1.5f);
            square.setOutlineColor(fill);
            window.draw(square);
        }
    }

    void DefaultScreen::beginPlaneDrag(ISceneObject *object, int plane, const sf::Vector2i &mouse)
    {
        if (!object)
            return;
        this->_planeDragActive = true;
        this->_planeDragMoved = false;
        this->_planeDragTarget = object;
        this->_planeDragPlane = plane;
        this->_planeDragObjStart = object->getPosition();
        this->_planeDragNormal = this->gizmoAxisDir((plane + 2) % 3);
        Vector3f grab;
        if (this->rayPlanePoint(mouse, this->_planeDragObjStart, this->_planeDragNormal, grab))
            this->_planeDragOffset = this->_planeDragObjStart - grab;
        else
            this->_planeDragOffset = {0.0f, 0.0f, 0.0f};
    }

    void DefaultScreen::applyPlaneDrag(const sf::Vector2i &mouse)
    {
        if (!this->_planeDragTarget)
            return;
        Vector3f point;
        if (!this->rayPlanePoint(mouse, this->_planeDragObjStart, this->_planeDragNormal, point))
            return;
        Vector3f newPos = point + this->_planeDragOffset;

        const Vector3f dirA = this->gizmoAxisDir(this->_planeDragPlane);
        const Vector3f dirB = this->gizmoAxisDir((this->_planeDragPlane + 1) % 3);
        const Vector3f delta = newPos - this->_planeDragObjStart;
        float da = dot(delta, dirA);
        float db = dot(delta, dirB);
        if (snapHeld())
        {
            da = snapTo(da, SNAP_MOVE);
            db = snapTo(db, SNAP_MOVE);
            newPos = this->_planeDragObjStart + dirA * da + dirB * db;
        }
        this->_planeDragTarget->setLocalPosition(newPos);
        this->_planeDragMoved = true;
        this->_gizmoReadout = fmtNum(da) + ", " + fmtNum(db);
        this->markViewportBvhDirty();
        this->forceViewportRetrace();
    }

    void DefaultScreen::endPlaneDrag()
    {
        if (!this->_planeDragActive)
            return;
        this->_planeDragActive = false;
        if (this->_planeDragMoved && this->_planeDragTarget)
        {
            this->_objectPanel.rebuild(this->_planeDragTarget);
            this->markViewportBvhDirty();
            this->forceViewportRetrace();
        }
        this->_planeDragMoved = false;
        this->_planeDragTarget = nullptr;
        this->_planeDragPlane = -1;
        this->_gizmoReadout.clear();
    }

    bool DefaultScreen::pickUniformScale(const sf::Vector2i &mouse) const
    {
        ISceneObject *object = this->singleSelectedObject();
        if (this->_editMode || !object)
            return (false);
        if (!this->_rendererPanel.viewportBounds.contains(mouse))
            return (false);
        sf::Vector2f center;
        if (!this->projectToViewport(object->getPosition(), center))
            return (false);
        const float dx = static_cast<float>(mouse.x) - center.x;
        const float dy = static_cast<float>(mouse.y) - center.y;
        return (dx * dx + dy * dy <= 8.0f * 8.0f);
    }

    void DefaultScreen::beginUniformScaleDrag(ISceneObject *object, const sf::Vector2i &mouse)
    {
        if (!object)
            return;
        this->_uscaleDragActive = true;
        this->_uscaleDragMoved = false;
        this->_uscaleDragTarget = object;
        this->_uscaleStartScale = object->getLocalScale();
        if (!this->projectToViewport(object->getPosition(), this->_uscaleCenter))
            this->_uscaleCenter = {static_cast<float>(mouse.x), static_cast<float>(mouse.y)};
        const float dx = static_cast<float>(mouse.x) - this->_uscaleCenter.x;
        const float dy = static_cast<float>(mouse.y) - this->_uscaleCenter.y;
        this->_uscaleGrabDist = std::max(6.0f, std::sqrt(dx * dx + dy * dy));
    }

    void DefaultScreen::applyUniformScaleDrag(const sf::Vector2i &mouse)
    {
        if (!this->_uscaleDragTarget)
            return;
        const float dx = static_cast<float>(mouse.x) - this->_uscaleCenter.x;
        const float dy = static_cast<float>(mouse.y) - this->_uscaleCenter.y;
        float factor = std::sqrt(dx * dx + dy * dy) / this->_uscaleGrabDist;
        if (snapHeld())
            factor = std::max(SNAP_SCALE, snapTo(factor, SNAP_SCALE));
        Vector3f scale = {
            std::max(0.001f, this->_uscaleStartScale.x * factor),
            std::max(0.001f, this->_uscaleStartScale.y * factor),
            std::max(0.001f, this->_uscaleStartScale.z * factor),
        };
        this->_uscaleDragTarget->setLocalScale(scale);
        this->_uscaleDragMoved = true;
        this->_gizmoReadout = fmtNum(factor) + "x";
        this->markViewportBvhDirty();
        this->forceViewportRetrace();
    }

    void DefaultScreen::endUniformScaleDrag()
    {
        if (!this->_uscaleDragActive)
            return;
        this->_uscaleDragActive = false;
        if (this->_uscaleDragMoved && this->_uscaleDragTarget)
        {
            this->_objectPanel.rebuild(this->_uscaleDragTarget);
            this->markViewportBvhDirty();
            this->forceViewportRetrace();
        }
        this->_uscaleDragMoved = false;
        this->_uscaleDragTarget = nullptr;
        this->_gizmoReadout.clear();
    }

    bool DefaultScreen::pickViewRotation(const sf::Vector2i &mouse) const
    {
        ISceneObject *object = this->singleSelectedObject();
        if (this->_editMode || !object || !this->_coreAccess)
            return (false);
        if (!this->_rendererPanel.viewportBounds.contains(mouse))
            return (false);
        IScene *scene = this->_coreAccess->getScene();
        if (!scene)
            return (false);
        const Vector3f objPos = object->getPosition();
        const ICamera &camera = scene->getCamera();
        const Vector3f right = camera.getRight();
        const Vector3f up = right.cross(camera.getForward()).unit_vector();
        const Vector3f toCam = objPos - camera.getPosition();
        const float radius = std::sqrt(dot(toCam, toCam)) * 0.19f * 1.28f;
        const sf::Vector2f m(static_cast<float>(mouse.x), static_cast<float>(mouse.y));
        constexpr float PI = 3.14159265358979323846f;
        float best = 8.0f;
        sf::Vector2f prev;
        bool havePrev = false;
        for (int i = 0; i <= 48; ++i)
        {
            const float theta = 2.0f * PI * static_cast<float>(i) / 48.0f;
            const Vector3f world = objPos + (right * std::cos(theta) + up * std::sin(theta)) * radius;
            sf::Vector2f screen;
            const bool ok = this->projectToViewport(world, screen);
            if (ok && havePrev)
            {
                const sf::Vector2f ab(screen.x - prev.x, screen.y - prev.y);
                const sf::Vector2f ap(m.x - prev.x, m.y - prev.y);
                const float len2 = ab.x * ab.x + ab.y * ab.y;
                float t = (len2 > 1e-6f) ? (ap.x * ab.x + ap.y * ab.y) / len2 : 0.0f;
                t = std::max(0.0f, std::min(1.0f, t));
                const float px = prev.x + ab.x * t, py = prev.y + ab.y * t;
                best = std::min(best, std::hypot(m.x - px, m.y - py));
            }
            prev = screen;
            havePrev = ok;
        }
        return (best < 7.0f);
    }

    void DefaultScreen::drawViewRotationRing(sf::RenderWindow &window) const
    {
        ISceneObject *object = this->singleSelectedObject();
        if (this->_editMode || !object || !this->_coreAccess)
            return;
        IScene *scene = this->_coreAccess->getScene();
        if (!scene)
            return;
        const Vector3f objPos = object->getPosition();
        const ICamera &camera = scene->getCamera();
        const Vector3f right = camera.getRight();
        const Vector3f up = right.cross(camera.getForward()).unit_vector();
        const Vector3f toCam = objPos - camera.getPosition();
        const float radius = std::sqrt(dot(toCam, toCam)) * 0.19f * 1.28f;
        constexpr float PI = 3.14159265358979323846f;
        const sf::Color color = this->_viewRotActive ? sf::Color(255, 235, 90) : sf::Color(210, 210, 220);
        const float thick = this->_viewRotActive ? 3.0f : 2.0f;
        sf::Vector2f prev;
        bool havePrev = false;
        for (int i = 0; i <= 48; ++i)
        {
            const float theta = 2.0f * PI * static_cast<float>(i) / 48.0f;
            const Vector3f world = objPos + (right * std::cos(theta) + up * std::sin(theta)) * radius;
            sf::Vector2f screen;
            const bool ok = this->projectToViewport(world, screen);
            if (ok && havePrev)
            {
                const sf::Vector2f d(screen.x - prev.x, screen.y - prev.y);
                const float len = std::sqrt(d.x * d.x + d.y * d.y);
                if (len >= 0.5f)
                {
                    sf::RectangleShape seg({len, thick});
                    seg.setOrigin(0.0f, thick / 2.0f);
                    seg.setPosition(prev);
                    seg.setRotation(std::atan2(d.y, d.x) * 180.0f / PI);
                    seg.setFillColor(color);
                    window.draw(seg);
                }
            }
            prev = screen;
            havePrev = ok;
        }
    }

    void DefaultScreen::beginViewRotationDrag(ISceneObject *object, const sf::Vector2i &mouse)
    {
        if (!object || !this->_coreAccess)
            return;
        IScene *scene = this->_coreAccess->getScene();
        if (!scene)
            return;
        this->_viewRotActive = true;
        this->_viewRotMoved = false;
        this->_viewRotValid = false;
        this->_viewRotTarget = object;
        this->_viewRotStartRot = object->getLocalRotation();
        this->_viewRotObjPos = object->getPosition();
        this->_viewRotAxis = scene->getCamera().getForward();
        Vector3f grab;
        if (this->rayPlanePoint(mouse, this->_viewRotObjPos, this->_viewRotAxis, grab))
        {
            const Vector3f gv = grab - this->_viewRotObjPos;
            const float len = std::sqrt(dot(gv, gv));
            if (len > 1e-5f)
            {
                this->_viewRotGrabVec = gv * (1.0f / len);
                this->_viewRotValid = true;
            }
        }
    }

    void DefaultScreen::applyViewRotationDrag(const sf::Vector2i &mouse)
    {
        if (!this->_viewRotTarget || !this->_viewRotValid)
            return;
        Vector3f point;
        if (!this->rayPlanePoint(mouse, this->_viewRotObjPos, this->_viewRotAxis, point))
            return;
        const Vector3f cv = point - this->_viewRotObjPos;
        const float len = std::sqrt(dot(cv, cv));
        if (len < 1e-5f)
            return;
        const Vector3f cur = cv * (1.0f / len);
        constexpr float PI = 3.14159265358979323846f;
        const float sine = dot(this->_viewRotGrabVec.cross(cur), this->_viewRotAxis);
        const float cosine = dot(this->_viewRotGrabVec, cur);
        float angleDeg = std::atan2(sine, cosine) * 180.0f / PI;
        if (snapHeld())
            angleDeg = snapTo(angleDeg, SNAP_ROT);
        this->_viewRotTarget->setLocalRotation(eulerCompose(this->_viewRotStartRot, this->_viewRotAxis, angleDeg));
        this->_viewRotMoved = true;
        this->_gizmoReadout = fmtNum(angleDeg) + " deg";
        this->markViewportBvhDirty();
        this->forceViewportRetrace();
    }

    void DefaultScreen::endViewRotationDrag()
    {
        if (!this->_viewRotActive)
            return;
        this->_viewRotActive = false;
        if (this->_viewRotMoved && this->_viewRotTarget)
        {
            this->_objectPanel.rebuild(this->_viewRotTarget);
            this->markViewportBvhDirty();
            this->forceViewportRetrace();
        }
        this->_viewRotMoved = false;
        this->_viewRotValid = false;
        this->_viewRotTarget = nullptr;
        this->_gizmoReadout.clear();
    }

    bool DefaultScreen::computeMarker(const sf::Vector2i &mouse, Vector3f &out)
    {
        IScene *scene = this->_coreAccess ? this->_coreAccess->getScene() : nullptr;
        if (!scene)
            return (false);
        sf::Vector2i pixel;
        if (!this->_rendererPanel.getViewportPixel(mouse, pixel))
            return (false);
        const ICamera &camera = scene->getCamera();
        const Vector2i resolution = camera.getResolution();
        const Ray ray = ViewportHelper::rayThroughPixel(camera, pixel.x, pixel.y, resolution.x, resolution.y);

        Intersection hit;
        if (scene->intersect(ray, 0.001f, std::numeric_limits<float>::infinity(), hit))
        {
            out = hit.point;
            return (true);
        }
        if (std::fabs(ray.direction.z) > 1e-6f)
        {
            const float t = -ray.origin.z / ray.direction.z;
            if (t > 0.0f)
            {
                out = ray.origin + ray.direction * t;
                return (true);
            }
        }
        out = ray.origin + ray.direction * 20.0f;
        return (true);
    }

    void DefaultScreen::placeMarker(const sf::Vector2i &mouse)
    {
        Vector3f position;
        if (!this->computeMarker(mouse, position))
            return;
        this->_markerActive = true;
        this->_markerPos = position;
        this->_toastManager.push("Marker placed", "New primitives from the Add menu will spawn here.", ToastType::INFO);
    }

    bool DefaultScreen::markerWindowPos(sf::Vector2f &out) const
    {
        if (!this->_markerActive || !this->_coreAccess)
            return (false);
        IScene *scene = this->_coreAccess->getScene();
        if (!scene)
            return (false);
        const ICamera &camera = scene->getCamera();
        const Vector2i resolution = camera.getResolution();
        sf::Vector2i pixel;
        if (!ViewportHelper::projectToPixel(camera, this->_markerPos, resolution.x, resolution.y, pixel))
            return (false);
        const sf::IntRect &bounds = this->_rendererPanel.viewportBounds;
        const float scale = this->_rendererPanel.viewportScale;
        out.x = static_cast<float>(bounds.left) + static_cast<float>(pixel.x) * scale;
        out.y = static_cast<float>(bounds.top) + static_cast<float>(pixel.y) * scale;
        return (true);
    }

    void DefaultScreen::drawMarker(sf::RenderWindow &window) const
    {
        sf::Vector2f p;
        if (!this->markerWindowPos(p))
            return;
        const sf::IntRect &bounds = this->_rendererPanel.viewportBounds;
        if (!bounds.contains(static_cast<int>(p.x), static_cast<int>(p.y)))
            return;

        const sf::Color color(255, 210, 40);
        const float len = 9.0f;
        const float thick = 2.0f;
        sf::RectangleShape h({2.0f * len, thick});
        h.setOrigin(len, thick / 2.0f);
        h.setPosition(p);
        h.setFillColor(color);
        sf::RectangleShape v({thick, 2.0f * len});
        v.setOrigin(thick / 2.0f, len);
        v.setPosition(p);
        v.setFillColor(color);
        sf::CircleShape ring(5.0f);
        ring.setOrigin(5.0f, 5.0f);
        ring.setPosition(p);
        ring.setFillColor(sf::Color::Transparent);
        ring.setOutlineThickness(2.0f);
        ring.setOutlineColor(color);
        window.draw(h);
        window.draw(v);
        window.draw(ring);
    }

    void DefaultScreen::addPrimitiveAtMarker(const std::string &type)
    {
        if (!this->_coreAccess)
            return;
        IScene *scene = this->_coreAccess->getScene();
        if (!scene)
            return;
        scene->addDefaultPrimitive(type);
        if (this->_markerActive)
        {
            const std::vector<IPrimitive *> &primitives = scene->getPrimitives();
            if (!primitives.empty())
            {
                ISceneObject *object = dynamic_cast<ISceneObject *>(primitives.back());
                if (object)
                    object->setLocalPosition(this->_markerPos);
            }
        }
    }

    void DefaultScreen::drawAxisGizmo(sf::RenderWindow &window) const
    {
        if (!this->_coreAccess || !this->_font)
            return;
        IScene *scene = this->_coreAccess->getScene();
        if (!scene)
            return;
        const sf::IntRect &bounds = this->_rendererPanel.viewportBounds;
        if (bounds.width <= 0 || bounds.height <= 0)
            return;

        const ICamera &camera = scene->getCamera();
        const Vector3f forward = camera.getForward();
        const Vector3f right = camera.getRight();
        const Vector3f up = right.cross(forward).unit_vector();

        constexpr float PI = 3.14159265358979323846f;
        const float radius = 26.0f;
        const float margin = 18.0f;
        const sf::Vector2f center(static_cast<float>(bounds.left + bounds.width) - margin - radius,
            static_cast<float>(bounds.top) + margin + radius);

        // Faint backing disc for contrast over any scene colour.
        sf::CircleShape disc(radius + 9.0f);
        disc.setOrigin(radius + 9.0f, radius + 9.0f);
        disc.setPosition(center);
        disc.setFillColor(sf::Color(18, 20, 26, 140));
        window.draw(disc);

        struct GizmoAxis { Vector3f dir; sf::Color color; const char *label; float depth; };
        GizmoAxis axes[3] = {
            {{1.0f, 0.0f, 0.0f}, sf::Color(235, 80, 80), "X", 0.0f},
            {{0.0f, 1.0f, 0.0f}, sf::Color(95, 205, 100), "Y", 0.0f},
            {{0.0f, 0.0f, 1.0f}, sf::Color(95, 160, 245), "Z", 0.0f},
        };
        for (GizmoAxis &a : axes)
            a.depth = dot(a.dir, forward);
        std::sort(axes, axes + 3, [](const GizmoAxis &a, const GizmoAxis &b) { return a.depth > b.depth; });

        for (const GizmoAxis &a : axes)
        {
            const float gx = dot(a.dir, right);
            const float gy = -dot(a.dir, up);
            const sf::Vector2f tip(center.x + gx * radius, center.y + gy * radius);
            const sf::Vector2f d(tip.x - center.x, tip.y - center.y);
            const float len = std::sqrt(d.x * d.x + d.y * d.y);

            if (len > 0.5f)
            {
                sf::RectangleShape line({len, 2.5f});
                line.setOrigin(0.0f, 1.25f);
                line.setPosition(center);
                line.setRotation(std::atan2(d.y, d.x) * 180.0f / PI);
                line.setFillColor(a.color);
                window.draw(line);
            }
            sf::CircleShape knob(4.5f);
            knob.setOrigin(4.5f, 4.5f);
            knob.setPosition(tip);
            knob.setFillColor(a.color);
            window.draw(knob);

            sf::Text label(a.label, *this->_font, 12);
            label.setFillColor(sf::Color::White);
            const sf::FloatRect lb = label.getLocalBounds();
            label.setOrigin(lb.left + lb.width / 2.0f, lb.top + lb.height / 2.0f);
            const sf::Vector2f nd = (len > 0.5f) ? sf::Vector2f(d.x / len, d.y / len) : sf::Vector2f(0.0f, 0.0f);
            label.setPosition(tip.x + nd.x * 9.0f, tip.y + nd.y * 9.0f);
            window.draw(label);
        }
    }

    void DefaultScreen::syncVertexNavigator()
    {
        IEditablePrimitive *editable = this->_editMode ? this->_editTarget : this->editableFromSelection();
        if (editable && editable->getVertexCount() > 0)
        {
            const int count = static_cast<int>(editable->getVertexCount());
            const int index = (this->_editMode && this->_selectedVertex >= 0) ? this->_selectedVertex : 0;
            this->_objectPanel.setVertexNavigator(true, index, count);
        }
        else
        {
            this->_objectPanel.setVertexNavigator(false, 0, 0);
        }
    }

    void DefaultScreen::navigateVertex(int direction)
    {
        IEditablePrimitive *editable = this->_editMode ? this->_editTarget : this->editableFromSelection();
        if (!editable)
            return;
        if (!this->_editMode)
        {
            const ISceneObject *object = const_cast<HierarchyPanel &>(this->_hierarchyPanel).tryCast<const ISceneObject>();
            this->enterEditMode(object, editable);
        }
        const int count = static_cast<int>(this->_editTarget->getVertexCount());
        if (count == 0)
            return;
        if (this->_selectedVertex < 0)
            this->_selectedVertex = (direction >= 0) ? 0 : count - 1;
        else
            this->_selectedVertex = ((this->_selectedVertex + direction) % count + count) % count;
        this->syncVertexEditorField();
        this->forceViewportRetrace();
    }

    void DefaultScreen::drawEditOverlay(sf::RenderWindow &window)
    {
        if (!this->_editMode || !this->_editTarget || !this->_font)
            return;

        const sf::IntRect &bounds = this->_rendererPanel.viewportBounds;

        // Persistent "Edit Mode" banner over the top-left of the viewport.
        const std::string name = this->_editObject ? this->_editObject->getName() : "object";
        sf::Text banner("Edit Mode  —  " + name + "    (Tab/Esc: exit    drag: move    X/Y/Z: lock axis)", *this->_font, 13);
        banner.setFillColor(theme::TEXT_WHITE);
        banner.setPosition(static_cast<float>(bounds.left) + 10.0f, static_cast<float>(bounds.top) + 7.0f);
        const sf::FloatRect textBounds = banner.getLocalBounds();
        sf::RectangleShape bannerBg({textBounds.width + 20.0f, 24.0f});
        bannerBg.setPosition(static_cast<float>(bounds.left) + 6.0f, static_cast<float>(bounds.top) + 6.0f);
        bannerBg.setFillColor(theme::withAlpha(theme::ACCENT, 210));
        window.draw(bannerBg);
        window.draw(banner);

        const sf::Color baseColor(0, 170, 255);
        const sf::Color hoverColor(255, 255, 255);
        const sf::Color selectedColor(255, 30, 30);   // bright red: the vertex being edited
        const std::size_t MAX_HANDLES = 4000;
        const std::size_t count = this->_editTarget->getVertexCount();
        std::size_t drawn = 0;
        for (std::size_t i = 0; i < count; ++i)
        {
            sf::Vector2f handle;
            if (!this->vertexHandleWindowPos(i, handle))
                continue;
            const bool selected = static_cast<int>(i) == this->_selectedVertex;
            const bool hovered = static_cast<int>(i) == this->_hoverVertex;
            const float radius = selected ? 6.5f : 4.0f;
            sf::CircleShape dot(radius);
            dot.setOrigin(radius, radius);
            dot.setPosition(handle);
            dot.setFillColor(selected ? selectedColor : (hovered ? hoverColor : baseColor));
            dot.setOutlineThickness(selected ? 2.0f : 1.0f);
            dot.setOutlineColor(selected ? sf::Color(255, 255, 255) : theme::OUTLINE);
            window.draw(dot);
            if (++drawn >= MAX_HANDLES)
                break;
        }
    }
}
