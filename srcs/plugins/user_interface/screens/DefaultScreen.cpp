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
#include <limits>

#include "../EventRouter.hpp"
#include "../components/menu/Menu.hpp"
#include "../Theme.hpp"
#include "../ViewportHelper.hpp"

namespace rc
{
    constexpr float MENU_HEIGHT = 28.0f;
    constexpr float SIDEBAR_MIN = 220.0f;   // narrowest the sidebar may be dragged
    constexpr float VIEWPORT_MIN = 360.0f;  // space always kept for the viewport

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
                this->_toastManager.push("Server started !", "Other users can join :!", ToastType::SUCCESS);
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
        this->_menuBar.setFont(*this->_font);

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
            // Read the name before removeObject() frees the object. The panel
            // skips descendants covered by a deleted ancestor, so `payload` is
            // always still alive here.
            const std::string name = payload->getName();
            scene->removeObject(const_cast<ISceneObject *>(payload));
            this->markViewportBvhDirty();
            this->_toastManager.push("Object deleted", name + " has been removed from the scene.", ToastType::SUCCESS);
        });

        this->_cameraPanel.setFont(*this->_font);
        this->_objectPanel.setFont(*this->_font);
        this->_objectPanel.onSceneMutated = [this]
        {
            // Rebuild the scene BVH AND force the viewport to re-trace: the
            // renderer only re-runs its geometry pass on a camera/selection
            // change, so without this a transform edit (position/rotation/scale,
            // incl. the -/+ size buttons) would rebuild the BVH but keep showing
            // the cached shape until the camera moved.
            this->markViewportBvhDirty();
            this->forceViewportRetrace();
        };
        // Keyboard edit of the selected vertex's world coordinates, kept in sync
        // with dragging. Applies straight onto the editable primitive.
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
        // -/+ buttons while a vertex is selected: move only that vertex, away
        // from (factor > 1) or toward (factor < 1) the shape's centroid, so the
        // selected point grows/shrinks while the rest of the geometry stays put.
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
        // Object-panel arrows: step to the previous / next vertex of the shape.
        this->_objectPanel.onVertexNavigate = [this](int direction)
        {
            this->navigateVertex(direction);
        };
        // "Convert to Mesh": bake the selected analytic primitive into an
        // editable triangle mesh and select the result.
        this->_objectPanel.onConvertToMesh = [this]
        {
            this->convertSelectionToMesh();
        };
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

        this->_menuBar.update(mouse);

        if (this->_menuBar.isOpen())
        {
            this->applyCursor(window, this->_menuBar.getCursor());
            return;
        }

        this->_rendererPanel.update(mouse);
        if (cursorType == CursorType::ARROW && this->_rendererPanel.getCursor() != CursorType::ARROW)
            cursorType = this->_rendererPanel.getCursor();

        if (this->_viewMode == ViewMode::RENDERING)
        {
            this->applyCursor(window, cursorType);
            return;
        }

        this->updateViewportCamera(window);

        this->layoutSidebarResize(window);

        this->_hierarchyPanel.setScene(this->_coreAccess ? this->_coreAccess->getScene() : nullptr);

        // Leave edit mode if the selection changed away from the edited object
        // (hierarchy reselection, deletion, multi-select). Guards against acting
        // on a stale/freed _editTarget.
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

        // The sidebar splitter takes precedence over the viewport cross so that
        // grabbing the edge always shows the resize cursor.
        this->_sidebarResize.update(mouse);
        if (this->_sidebarResize.getCursor() != CursorType::ARROW)
            cursorType = this->_sidebarResize.getCursor();

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
            this->drawMarker(window);
        }

        this->_menuBar.layout(static_cast<float>(window.getSize().x));
        window.draw(this->_menuBar);

        this->_toastManager.draw(window);
    }

    void DefaultScreen::drawRenderer(sf::RenderWindow &window, ISceneRenderer *renderer)
    {
        sf::Vector2u windowSize = window.getSize();
        this->_rendererPanel.layout(this->_sidebarWidth, MENU_HEIGHT, static_cast<float>(windowSize.x) - this->_sidebarWidth, static_cast<float>(windowSize.y) - MENU_HEIGHT);
        this->_rendererPanel.updateRender(renderer->getRender());
        window.draw(this->_rendererPanel);
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

    void DefaultScreen::handleEvent(sf::RenderWindow &window, sf::Event &event)
    {
        const sf::Vector2i mouse = sf::Mouse::getPosition(window);

        if (this->_joinClusterWindow.running)
            return;

        if (this->_exploratorWindow.running)
            return;

        if (this->_loadWindow.running)
            return;

        // Right mouse drives the viewport camera rotation independently of the
        // component routing below, but only when the drag begins over the
        // viewport itself - a right-press on a panel must not grab the camera.
        if (event.type == sf::Event::MouseButtonPressed &&
            event.mouseButton.button == sf::Mouse::Right &&
            this->isViewportCaptured(mouse))
        {
            // Shift + right-click drops a 3D marker under the cursor instead of
            // orbiting; the next primitive added from the menu spawns there.
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::LShift)
                || sf::Keyboard::isKeyPressed(sf::Keyboard::RShift))
            {
                this->placeMarker(mouse);
                return;
            }
            this->_rightMouseHeld = true;
            this->_lastMouse = sf::Mouse::getPosition(window);
        }

        if (event.type == sf::Event::MouseButtonReleased &&
            event.mouseButton.button == sf::Mouse::Right)
        {
            this->_rightMouseHeld = false;
        }

        // Latch fly-camera keys here, while the event is fresh and before the
        // frame's (possibly slow) render, so presses/releases are never missed.
        this->trackFlyKeys(event, mouse);

        const bool viewportMode = this->_viewMode == ViewMode::VIEWPORT;

        // Vertex edit mode: keep driving an in-progress drag no matter what the
        // cursor is now over, and end it on release. Handled before component
        // routing so a fast drag that strays onto a panel is never dropped.
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

        // Tab toggles vertex edit mode for a selected editable primitive;
        // Escape leaves it. Suppressed while a field/pop-up owns the input.
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

        // Build the set of top-level components that are live in the current view
        // mode, then let the router pick the single best one for this event
        // (menu bar and open pop-ups win over the panels and the viewport).
        std::vector<Component *> candidates = {&this->_menuBar, &this->_rendererPanel};

        if (viewportMode)
        {
            this->layoutSidebarResize(window);
            candidates.push_back(&this->_sidebarResize);
            this->_sidebar.collectComponents(candidates);
        }

        const Component *consumer = EventRouter::route(candidates, event, mouse);

        // Delete / Suppr with the cursor outside the hierarchy panel: the key
        // never reaches the panel (its ScrollView only forwards keys while
        // hovered), so fall back to deleting the current selection here - the
        // same shape as the unclaimed-left-click viewport fallback below. A live
        // text field or the hovered panel consumes the key first (consumer set),
        // which keeps Delete editing text / handled once.
        if (viewportMode && consumer == nullptr
            && event.type == sf::Event::KeyPressed && event.key.code == sf::Keyboard::Delete)
        {
            this->_hierarchyPanel.deleteSelection();
        }

        if (this->_hierarchyPanel.consumeSelectionChanged() || this->_objectPanel.consumeMaterialChanged())
        {
            this->syncSelectionToRenderer();
        }

        // Fall back to viewport picking only when no UI component claimed the
        // click, so clicks on panels or pop-ups no longer leak to the scene.
        if (viewportMode && consumer == nullptr &&
            event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Left)
        {
            if (mouse.x > this->_sidebarWidth && mouse.y >= static_cast<int>(MENU_HEIGHT))
            {
                if (this->_editMode)
                {
                    // In edit mode a left click grabs the nearest vertex handle
                    // (object selection is intentionally disabled); a miss just
                    // clears the vertex selection.
                    const int vertex = this->pickVertexHandle(mouse);
                    if (vertex >= 0)
                        this->beginVertexDrag(vertex);
                    else
                    {
                        this->_selectedVertex = -1;
                        this->syncVertexEditorField();
                    }
                }
                else
                {
                    this->updateSelectionFromClick(mouse);

                    // Double-click an editable primitive to jump into edit mode.
                    const IPrimitive *prim = this->_hierarchyPanel.tryCast<const IPrimitive>();
                    const ISceneObject *obj = this->_hierarchyPanel.tryCast<const ISceneObject>();
                    IEditablePrimitive *editable = prim ? dynamic_cast<IEditablePrimitive *>(const_cast<IPrimitive *>(prim)) : nullptr;
                    if (editable && obj && obj == this->_editClickObject
                        && this->_editClickClock.getElapsedTime().asMilliseconds() < 350)
                        this->enterEditMode(obj, editable);
                    this->_editClickObject = obj;
                    this->_editClickClock.restart();

                    // Left-drag the object just clicked to move it in space (a
                    // plain click with no drag is still just a selection). Skipped
                    // in edit mode and when Ctrl (multi-select) is held.
                    const bool ctrl = sf::Keyboard::isKeyPressed(sf::Keyboard::LControl)
                        || sf::Keyboard::isKeyPressed(sf::Keyboard::RControl);
                    const ISceneObject *underCursor = this->pickObjectAt(mouse);
                    if (!this->_editMode && !ctrl && underCursor)
                        this->beginObjectDrag(const_cast<ISceneObject *>(underCursor), mouse);
                }
            }
        }

        // Track what's under the cursor so the viewport can highlight it. In edit
        // mode this highlights the nearest vertex handle instead of an object.
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
    }

    bool DefaultScreen::isViewportCaptured(const sf::Vector2i &mouse)
    {
        if (this->_viewMode != ViewMode::VIEWPORT)
            return (false);
        if (!this->_rendererPanel.viewportBounds.contains(mouse))
            return (false);

        // A menu, pop-up, in-progress drag or focused text field anywhere in the
        // UI owns the input even when the cursor sits over the render, so the
        // viewport must not steal keys/clicks while any of them is live.
        if (this->_menuBar.isCapturing())
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

        if (this->_rightMouseHeld)
        {
            sf::Vector2i delta = mouse - this->_lastMouse;
            this->_lastMouse = mouse;

            Vector3f rot = camera.getRotation();

            rot.z += delta.x * 0.1f;
            rot.x += delta.y * 0.1f;
            rot.y = 0.0f;

            camera.setRotation(rot);
        }

        // Real elapsed frame time drives the motion so speed is constant
        // regardless of frame rate or how long the render took. The clock is
        // restarted every frame (this runs each viewport frame), and dt is
        // clamped so a single slow frame -- e.g. a full-resolution refine --
        // can't fling the camera across the scene.
        const float dt = std::clamp(this->_frameClock.restart().asSeconds(), 0.0f, 0.1f);

        // Which fly keys are held is latched from events (trackFlyKeys), not
        // polled here, so a press captured over the viewport keeps moving the
        // camera while held even if the cursor later drifts off the viewport.
        // Reconcile each latch against the physical key so a missed key-up (a
        // modal window stealing focus mid-hold, etc.) can never leave the camera
        // stuck moving -- this only clears latches, it never starts a fly.
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
        // Losing window focus (Alt-Tab, etc.) can swallow the matching key-up, so
        // clear everything to avoid a stuck, forever-moving camera.
        if (event.type == sf::Event::LostFocus)
        {
            this->resetFlyKeys();
            return;
        }
        // Releases always count, so a key can never stick regardless of where the
        // cursor is; presses only start a fly while the viewport owns input, so
        // typing into a focused field or panel never drives the camera.
        if (event.type == sf::Event::KeyReleased)
            this->setFlyKey(event.key.code, false);
        else if (event.type == sf::Event::KeyPressed && !this->_vertexDragActive &&
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
                this->_viewportBvhDirty = false;
            }
            this->_activeRenderer->renderScene(*this->_coreAccess->getScene());
        }
    }

    void DefaultScreen::markViewportBvhDirty()
    {
        this->_viewportBvhDirty = true;
        // A dirty BVH always means the geometry changed, so force the viewport to
        // re-trace: the ViewportRenderer otherwise only refreshes on a camera or
        // selection change and would keep showing the cached image (e.g. after
        // adding or deleting an object) until the camera moved.
        this->forceViewportRetrace();
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
        // tryCast is non-const; the panel is only inspected here.
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
        // Never calls into _editTarget here: the selection may have changed
        // because the object was deleted, so the pointer can already be stale.
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
            // Nearest to the cursor wins; ties (overlapping handles) go to the
            // vertex closest to the camera.
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

        // Drag in the plane through the vertex's start position, parallel to the
        // image plane (normal = camera forward) -- the intuitive default.
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
        // Refresh the hierarchy and select the new mesh so it is ready to edit.
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
        this->_objectDragTarget->setLocalPosition(grab + this->_objectDragOffset);
        this->_objectDragMoved = true;
        this->markViewportBvhDirty();
        this->forceViewportRetrace();
    }

    void DefaultScreen::endObjectDrag()
    {
        if (!this->_objectDragActive)
            return;
        this->_objectDragActive = false;
        // A pure click (no movement) leaves the object where it was; only refresh
        // the Object panel's position field when the object actually moved.
        if (this->_objectDragMoved && this->_objectDragTarget)
        {
            this->_objectPanel.rebuild(this->_objectDragTarget);
            this->markViewportBvhDirty();
            this->forceViewportRetrace();
        }
        this->_objectDragTarget = nullptr;
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

        // Prefer the point on the surface under the cursor.
        Intersection hit;
        if (scene->intersect(ray, 0.001f, std::numeric_limits<float>::infinity(), hit))
        {
            out = hit.point;
            return (true);
        }
        // Otherwise drop it on the ground plane (z = 0), or a fixed distance ahead.
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

    void DefaultScreen::syncVertexNavigator()
    {
        // The navigator is available whenever the selected object has vertices,
        // even before entering edit mode, so it doubles as the entry point.
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

        // Vertex handles. Drawing is capped so a dense mesh can't stall the UI;
        // off-screen / behind-camera vertices are skipped by the projection.
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
            // The selected handle is drawn larger with a white outline so the
            // bright-red dot stays legible over any surface colour.
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
