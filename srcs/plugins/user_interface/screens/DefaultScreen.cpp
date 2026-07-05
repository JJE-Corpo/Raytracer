//
// Created by jazema on 5/16/26.
//

#include "DefaultScreen.hpp"
#include "../../../common/scene/ILight.hpp"
#include "../../../common/Intersection.hpp"
#include "../../../common/scene/IPrimitive.hpp"
#include "../../../common/ISelectionAwareRenderer.hpp"
#include "../../../common/scene/IScene.hpp"
#include <algorithm>
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
    // Pixels a right-press may travel before it counts as a camera-rotate drag
    // rather than a click that opens the context menu.
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
                this->_coreAccess->getScene()->addDefaultPrimitive("plane");
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
                this->_coreAccess->getScene()->addDefaultPrimitive("sphere");
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
                this->_coreAccess->getScene()->addDefaultPrimitive("cylinder");
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
                this->_coreAccess->getScene()->addDefaultPrimitive("cone");
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
                this->_coreAccess->getScene()->addDefaultPrimitive("triangle");
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
                this->_coreAccess->getScene()->addDefaultPrimitive("cube");
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
                this->_coreAccess->getScene()->addDefaultPrimitive("fractal");
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
                this->_coreAccess->getScene()->addDefaultPrimitive("tanglecube");
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
                this->_coreAccess->getScene()->addDefaultPrimitive("torus");
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
            this->markViewportBvhDirty();
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

        // Keep the context menu's hover highlight live; it sits above the menu
        // bar and panels, so refresh it before the menu-bar early-return below.
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

        if (this->_viewMode == ViewMode::RENDERING)
        {
            this->applyCursor(window, cursorType);
            return;
        }

        this->updateViewportCamera(window);

        this->layoutSidebarResize(window);

        // Detect a scene replaced outside our own undo/redo (startup, Open, a
        // config reload). Drop selection/hover that pointed into the old scene
        // and start a fresh undo baseline on the new one. onSceneRestored keeps
        // _lastHistoryScene in step for our swaps, so those don't reach here.
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

        this->_menuBar.layout(static_cast<float>(window.getSize().x));
        window.draw(this->_menuBar);

        // Drawn after the menu bar so the popup sits on top of every panel; it
        // no-ops while closed.
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

        // Only leaves carry a visibility toggle, matching the eye button the
        // hierarchy shows for primitives and lights (groups just get Delete).
        const ObjectType type = object->getObjectType();
        if (type == ObjectType::PRIMITIVE || type == ObjectType::LIGHT)
        {
            const std::string hideLabel = object->isHidden() ? "Show" : "Hide";
            entries.emplace_back(hideLabel, [this]() { this->hideSelection(); });
        }
        // Grouping only makes sense for a real multi-selection: gather the
        // selected objects under one new group.
        if (this->_hierarchyPanel.getSelection().size() >= 2)
            entries.emplace_back("Group selection", [this]() { this->groupSelection(); });
        // Delete reuses the hierarchy's Delete path (selection-wide, with the
        // safe pointer cleanup and the "Object deleted" toast).
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

        // Snapshot the selection: reparenting mutates the scene graph.
        const std::vector<const ISceneObject *> selection = this->_hierarchyPanel.getSelection();
        if (selection.size() < 2)
            return;

        // Only move the "roots" of the selection. An object whose selected
        // ancestor is also in the batch already travels with that ancestor's
        // subtree, so moving it too would tear it out of its parent. This mirrors
        // the skip rule HierarchyPanel::deleteObjects uses.
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

        // Created only once there is something to hold, so no empty group is
        // left behind. reparent preserves each object's world transform.
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

        // Global keyboard shortcuts (save / undo / redo) run before any routing
        // so they win over the viewport and panels. handleShortcut returns true
        // once it has consumed the event; an undo/redo may have swapped the scene
        // object, so nothing below must touch the now-stale local state.
        if (this->handleShortcut(event))
            return;

        // Right mouse over the viewport is overloaded: a drag rotates the camera
        // (independently of the component routing below), while a plain click
        // (released without dragging past the threshold) opens the object
        // context menu. A right-press on a panel must not grab the camera, and
        // is handled by the hierarchy panel via consumeContextMenuRequest below.
        if (event.type == sf::Event::MouseButtonPressed &&
            event.mouseButton.button == sf::Mouse::Right)
        {
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

        // Latch fly-camera keys here, while the event is fresh and before the
        // frame's (possibly slow) render, so presses/releases are never missed.
        this->trackFlyKeys(event, mouse);

        // Build the set of top-level components that are live in the current view
        // mode, then let the router pick the single best one for this event
        // (menu bar and open pop-ups win over the panels and the viewport).
        const bool viewportMode = this->_viewMode == ViewMode::VIEWPORT;

        std::vector<Component *> candidates = {&this->_contextMenu, &this->_menuBar, &this->_rendererPanel};

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

        // A right-click on a hierarchy row asks for the context menu; open it at
        // the (window-space) cursor. The viewport right-click is handled above,
        // where the picked object is already known.
        if (const ISceneObject *ctxObject = this->_hierarchyPanel.consumeContextMenuRequest())
            this->openContextMenu(ctxObject, mouse);

        // Fall back to viewport picking only when no UI component claimed the
        // click, so clicks on panels or pop-ups no longer leak to the scene.
        if (viewportMode && consumer == nullptr &&
            event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Left)
        {
            if (mouse.x > this->_sidebarWidth && mouse.y >= static_cast<int>(MENU_HEIGHT))
                this->updateSelectionFromClick(mouse);
        }

        // Track what's under the cursor so the viewport can highlight it, the
        // same way the click fallback above picks the object to select.
        if (event.type == sf::Event::MouseMoved || event.type == sf::Event::MouseLeft)
        {
            if (viewportMode && consumer == nullptr && event.type == sf::Event::MouseMoved)
                this->updateHoverFromMouse(mouse);
            else
                this->clearHover();
        }

        // Fold whatever this event changed into the undo history. Only commit-
        // like events snapshot (a press for menu/context actions, a release to
        // end a drag, a key release for typed edits), and historyCapture() is a
        // no-op when nothing actually changed -- so a slider drag becomes a
        // single undo step and plain selections/camera moves add none.
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

        // A menu, pop-up, in-progress drag or focused text field anywhere in the
        // UI owns the input even when the cursor sits over the render, so the
        // viewport must not steal keys/clicks while any of them is live.
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
        // typing into a focused field or panel never drives the camera. A press
        // with Ctrl held is a shortcut (Ctrl+S/Z), not a fly key, so ignore it -
        // otherwise Ctrl+Z would also nudge the camera forward (Z == forward).
        if (event.type == sf::Event::KeyReleased)
            this->setFlyKey(event.key.code, false);
        else if (event.type == sf::Event::KeyPressed && !event.key.control &&
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
    }

    void DefaultScreen::triggerSave()
    {
        // A never-saved scene has no path yet, so route to the save dialog (same
        // flow as "Save as"); an already-saved one overwrites its file directly.
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

        // Ctrl+S saves regardless of focus (it never destroys work); the modal
        // windows already return earlier in handleEvent, so this can't fire while
        // the save/open dialog is up.
        if (event.key.code == sf::Keyboard::S)
        {
            this->triggerSave();
            return (true);
        }

        // Ctrl+Z / Ctrl+Shift+Z undo/redo, but only in the editing viewport and
        // never while a text field owns the keyboard (so typing keeps its own
        // meaning and the scene isn't swapped from under an active edit).
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

        // The previous scene object was deleted by the swap, so every cached
        // pointer into it (selection, hover, panels) must be dropped/rebound
        // before anything touches it again.
        this->_hierarchyPanel.applyViewportSelection({});
        this->_hierarchyPanel.setScene(scene);
        this->clearHover();
        this->syncSelectionToRenderer();
        this->markViewportBvhDirty();

        // This swap is ours, so keep the baseline tracker in step: otherwise
        // update() would see the changed pointer and wipe the history.
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
    }
}
