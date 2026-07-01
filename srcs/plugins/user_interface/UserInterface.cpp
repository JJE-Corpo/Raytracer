//
// Created by jazema on 4/21/26.
//

#include "UserInterface.hpp"
#include "../../common/IPlugin.hpp"
#include "../../common/ICoreAccess.hpp"
#include "../../common/scene/ILight.hpp"
#include "../../common/Intersection.hpp"
#include "../../common/scene/IPrimitive.hpp"
#include "../../common/ISelectionAwareRenderer.hpp"
#include "../../common/scene/IScene.hpp"
#include <algorithm>
#include <limits>

#include "VerticalLayout.hpp"
#include "EventRouter.hpp"
#include "components/menu/Menu.hpp"
#include "Theme.hpp"
#include "ViewportHelper.hpp"


namespace rc
{
    constexpr float MENU_HEIGHT = 28.0f;
    constexpr float SIDEBAR_MIN = 220.0f;   // narrowest the sidebar may be dragged
    constexpr float VIEWPORT_MIN = 360.0f;  // space always kept for the viewport

    UserInterface::UserInterface() : _running(false), _coreAccess(nullptr), _clusterClientLayout(), _loadWindow(), _exploratorWindow()
    {
    }

    UserInterface::~UserInterface()
    {
        this->UserInterface::destroy();
    }

    void UserInterface::buildUI()
    {
        this->_viewportBvhDirty = true;
        this->_toastManager.setFont(this->_font);

        this->_rendererPanel.setFont(this->_font);
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

                this->_exploratorWindow.create(this->_font, ExploratorMode::OPEN, this->_exploratorResult);
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

                    this->_exploratorWindow.create(this->_font, ExploratorMode::SAVE, this->_exploratorResult);
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

                this->_exploratorWindow.create(this->_font, ExploratorMode::SAVE, this->_exploratorResult);
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
                            this->_loadWindow.create(this->_font, *this->_coreAccess->getScene(), this->_coreAccess->loadNewScene(this->_exploratorResult));
                            this->markViewportBvhDirty();

                            this->_toastManager.push("Import successful", "Scene imported successfully.", ToastType::SUCCESS);
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

                this->_exploratorWindow.create(this->_font, ExploratorMode::OPEN, this->_exploratorResult);
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

        addMenu.items = { newPlane, newSphere, newCylinder, newCone, newTriangle, newCube, newFractal, newTanglecube, newTorus, newPointLight, newDirectionalLight };

        Menu renderMenu;
        renderMenu.setLabel("Render");

        MenuItem renderRender;
        renderRender.setLabel("Render");
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
            this->_joinClusterWindow.create(this->_font);
        };

        clusterMenu.items = { clusterHost, clusterConnect };

        this->_joinClusterWindow.windowCallback = [this](std::string ip, size_t port)
        {
            try
            {
                this->_coreAccess->getClusterModule()->joinCluster(ip, port);
                this->_clusterClientLayout.setClient(this->_coreAccess->getClusterModule()->getClusterClient());
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
        this->_menuBar.setFont(this->_font);

        this->_hierarchyPanel.setFont(this->_font);
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

        this->_cameraPanel.setFont(this->_font);
        this->_objectPanel.setFont(this->_font);
        this->_objectPanel.onSceneMutated = [this]
        {
            this->markViewportBvhDirty();
        };
        this->_materialPanel.setFont(this->_font);

        this->_sidebarResize.onResize = [this](float width)
        {
            this->_sidebarWidth = width;
        };

        this->setupSidebarSection(SidebarStack::HIERARCHY, "hierarchy", "Hierarchy", &this->_hierarchyPanel,
            [this](float x, float y, float w) { this->_hierarchyPanel.layout(x, y, w); },
            [this] { return this->_hierarchyPanel.height; });
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

    void UserInterface::setupSidebarSection(SidebarStack::Slot slot, const std::string &id, const std::string &title,
        Component *content, std::function<void(float, float, float)> layoutContent, std::function<float()> contentHeight)
    {
        Section &section = this->_sidebar.section(slot);
        section.id = id;
        section.setTitle(title);
        section.setFont(this->_font);
        section.body.content = content;
        section.body.layoutContent = std::move(layoutContent);
        section.body.contentHeight = std::move(contentHeight);
    }

    void UserInterface::setCursor(CursorType cursorType)
    {
        sf::Cursor &cursor = (cursorType == CursorType::CROSS ? this->_cursorViewport :
                        cursorType == CursorType::ARROW ? this->_cursorArrow :
                        cursorType == CursorType::HAND ? this->_cursorHand :
                        cursorType == CursorType::TEXT ? this->_cursorText :
                        cursorType == CursorType::RESIZE_H ? this->_cursorResize :
                        cursorType == CursorType::RESIZE_V ? this->_cursorResizeV :
                        this->_cursorNotAllowed);

        this->_window.setMouseCursor(cursor);
    }

    void UserInterface::updateUI()
    {
        sf::Vector2i mouse = sf::Mouse::getPosition(this->_window);
        CursorType cursorType = this->_menuBar.getCursor();

        if (this->_exploratorJustClosed && !this->_exploratorWindow.running)
        {
            this->_exploratorJustClosed = false;
            this->_exploratorOnClose();
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
            this->setCursor(this->_menuBar.getCursor());
            return;
        }

        this->_rendererPanel.update(mouse);
        if (cursorType == CursorType::ARROW && this->_rendererPanel.getCursor() != CursorType::ARROW)
            cursorType = this->_rendererPanel.getCursor();

        if (this->_viewMode == ViewMode::RENDERING)
        {
            this->setCursor(cursorType);
            return;
        }

        this->updateViewportCamera();

        this->layoutSidebarResize();

        this->_hierarchyPanel.setScene(this->_coreAccess ? this->_coreAccess->getScene() : nullptr);
        this->refreshSidebarVisibility();
        this->_sidebar.layout(this->_sidebarWidth, MENU_HEIGHT, static_cast<float>(this->_window.getSize().y));
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

        this->setCursor(cursorType);

        this->_toastManager.update();
    }

    void UserInterface::drawUI()
    {
        this->layoutSidebarResize();

        auto size = this->_window.getSize();

        sf::RectangleShape panel;
        panel.setPosition({0, MENU_HEIGHT});
        panel.setSize({this->_sidebarWidth, (float)size.y - MENU_HEIGHT});
        panel.setFillColor(theme::BG_PANEL);
        this->_window.draw(panel);

        if (this->_viewMode == ViewMode::RENDERING)
            return;

        this->refreshSidebarVisibility();
        this->_sidebar.layout(this->_sidebarWidth, MENU_HEIGHT, static_cast<float>(size.y));
        this->_sidebar.draw(this->_window);

        this->_window.draw(this->_sidebarResize);
    }

    void UserInterface::drawRenderer(ISceneRenderer *renderer)
    {
        sf::Vector2u windowSize = this->_window.getSize();
        this->_rendererPanel.layout(this->_sidebarWidth, MENU_HEIGHT, static_cast<float>(windowSize.x) - this->_sidebarWidth, static_cast<float>(windowSize.y) - MENU_HEIGHT);
        this->_rendererPanel.updateRender(renderer->getRender());
        this->_window.draw(this->_rendererPanel);
    }

    void UserInterface::syncSelectionToRenderer()
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

    void UserInterface::updateSelectionFromClick(const sf::Vector2i &mouse)
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

    void UserInterface::handleWindowEvent(const sf::Event &event)
    {
        const sf::Vector2i mouse = sf::Mouse::getPosition(this->_window);

        if (this->_joinClusterWindow.running)
            return;

        if (this->_exploratorWindow.running)
            return;

        if (this->_loadWindow.running)
            return;

        // Right mouse drives the viewport camera rotation independently of the
        // component routing below.
        if (event.type == sf::Event::MouseButtonPressed &&
            event.mouseButton.button == sf::Mouse::Right)
        {
            this->_rightMouseHeld = true;
            this->_lastMouse = sf::Mouse::getPosition(this->_window);
        }

        if (event.type == sf::Event::MouseButtonReleased &&
            event.mouseButton.button == sf::Mouse::Right)
        {
            this->_rightMouseHeld = false;
        }

        // Build the set of top-level components that are live in the current view
        // mode, then let the router pick the single best one for this event
        // (menu bar and open pop-ups win over the panels and the viewport).
        const bool viewportMode = this->_viewMode == ViewMode::VIEWPORT;

        std::vector<Component *> candidates = {&this->_menuBar, &this->_rendererPanel};

        if (viewportMode)
        {
            this->layoutSidebarResize();
            candidates.push_back(&this->_sidebarResize);
            this->_sidebar.collectComponents(candidates);
        }

        const Component *consumer = EventRouter::route(candidates, event, mouse);

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
                this->updateSelectionFromClick(mouse);
        }
    }

    void UserInterface::updateViewportCamera()
    {
        IScene *scene = _coreAccess->getScene();

        if (!scene)
            return;

        ICamera &camera = scene->getCamera();

        if (this->_rightMouseHeld)
        {
            sf::Vector2i mouse = sf::Mouse::getPosition(this->_window);
            sf::Vector2i delta = mouse - this->_lastMouse;
            this->_lastMouse = mouse;

            Vector3f rot = camera.getRotation();

            rot.z += delta.x * 0.1f;
            rot.x += delta.y * 0.1f;
            rot.y = 0.0f;

            camera.setRotation(rot);
        }

        Vector3f pos = camera.getPosition();
        Vector3f forward = camera.getForward();
        Vector3f right = camera.getRight();

        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Z))
            pos = pos + forward * this->_cameraSpeed;
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::S))
            pos = pos - forward * this->_cameraSpeed;
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::D))
            pos = pos + right * this->_cameraSpeed;
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Q))
            pos = pos - right * this->_cameraSpeed;
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Space))
            pos.z += this->_cameraSpeed;
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::LShift))
            pos.z -= this->_cameraSpeed;
        camera.setPosition(pos);
    }

    void UserInterface::eventLoop()
    {
        {
            std::lock_guard lock(this->_windowMutex);
            this->_window.setActive(true);
            this->_window.setFramerateLimit(60);
            this->_window.setMouseCursor(this->_cursorArrow);
        }
        while (!this->shouldExit())
        {
            sf::Event event;
            while (this->_window.pollEvent(event) && !this->shouldExit())
            {
                if (event.type == sf::Event::Closed)
                {
                    this->_running = false;
                    this->_coreAccess->stop();
                    // this->_window.close();
                    break;
                }
                if (event.type == sf::Event::Resized)
                {
                    sf::FloatRect visibleArea(0, 0, static_cast<float>(event.size.width), static_cast<float>(event.size.height));
                    this->_window.setView(sf::View(visibleArea));
                }
                if (this->_coreAccess->getClusterModule()->getClusterMode() == ClusterMode::CLIENT)
                {
                    this->_clusterClientLayout.handleEvent(this->_window, event);
                    continue;
                }
                handleWindowEvent(event);
            }
            if (this->shouldExit())
                break;
            {
                if (this->_coreAccess->getClusterModule()->getClusterMode() == ClusterMode::CLIENT)
                {
                    this->_clusterClientLayout.tick(this->_window);
                    continue;
                }
                ISceneRenderer *renderer = nullptr;

                if (this->_coreAccess && this->_viewMode == ViewMode::RENDERING && this->_coreAccess->getRenderer())
                    renderer = this->_coreAccess->getRenderer();
                else if (this->_coreAccess && this->_viewMode == ViewMode::VIEWPORT && this->_coreAccess->getViewportRenderer())
                {
                    renderer = this->_coreAccess->getViewportRenderer();
                    if (this->_viewportBvhDirty)
                    {
                        this->_coreAccess->getScene()->buildBvh();
                        this->_viewportBvhDirty = false;
                    }
                    renderer->renderScene(*this->_coreAccess->getScene());
                }

                std::lock_guard lock(this->_windowMutex);
                this->_window.clear(theme::BG_WINDOW);
                this->updateUI();
                this->drawUI();
                if (renderer)
                    this->drawRenderer(renderer);
                this->_menuBar.layout(this->_window.getSize().x);
                this->_window.draw(this->_menuBar);
                this->_toastManager.draw(this->_window);
                this->_window.display();
            }
        }
        this->_window.setActive(false);
    }

    void UserInterface::setupLayout(Layout &layout)
    {
        layout.setFont(this->_font);
        layout.exit = [this]
        {
            this->exit();
        };
        layout.shouldExit = [this]
        {
            return (this->shouldExit());
        };
    }

    void UserInterface::markViewportBvhDirty()
    {
        this->_viewportBvhDirty = true;
    }

    void UserInterface::layoutSidebarResize()
    {
        const sf::Vector2u size = this->_window.getSize();
        const float maxWidth = std::max(SIDEBAR_MIN + 40.f, static_cast<float>(size.x) - VIEWPORT_MIN);

        this->_sidebarWidth = std::clamp(this->_sidebarWidth, SIDEBAR_MIN, maxWidth);
        this->_sidebarResize.setRange(SIDEBAR_MIN, maxWidth);
        this->_sidebarResize.setBounds(this->_sidebarWidth, MENU_HEIGHT, static_cast<float>(size.y) - MENU_HEIGHT);
    }

    void UserInterface::refreshSidebarVisibility()
    {
        this->_sidebar.setVisible(SidebarStack::HIERARCHY, true);
        this->_sidebar.setVisible(SidebarStack::CAMERA, this->_hierarchyPanel.isCameraSelected());
        this->_sidebar.setVisible(SidebarStack::OBJECT, !this->_hierarchyPanel.getSelection().empty());
        this->_sidebar.setVisible(SidebarStack::MATERIAL, this->_hierarchyPanel.tryCast<const IPrimitive>() != nullptr);
    }

    void UserInterface::create(ICoreAccess &core_access)
    {
        this->_coreAccess = &core_access;
        this->_window.create({1280, 720}, "Raytracer UI grrahboom", sf::Style::Default);
        this->_window.setActive(false);
        this->_font.loadFromFile("assets/font.ttf");
        this->_cursorArrow.loadFromSystem(sf::Cursor::Arrow);
        this->_cursorHand.loadFromSystem(sf::Cursor::Hand);
        this->_cursorText.loadFromSystem(sf::Cursor::Text);
        this->_cursorNotAllowed.loadFromSystem(sf::Cursor::NotAllowed);
        this->_cursorViewport.loadFromSystem(sf::Cursor::Cross);
        this->_cursorResize.loadFromSystem(sf::Cursor::SizeHorizontal);
        this->_cursorResizeV.loadFromSystem(sf::Cursor::SizeVertical);
        this->setupLayout(this->_clusterClientLayout);
        this->setupLayout(this->_defaultLayout);
        this->_clusterClientLayout.setOnLeave([this]
        {
            try
            {
                this->_coreAccess->getClusterModule()->leaveCluster();
                this->_clusterClientLayout.setClient(nullptr);
            }
            catch (std::exception &e)
            {
                this->_toastManager.push("Cannot leave cluster", e.what(), ToastType::ERROR);
            }
        });
        this->_clusterClientLayout.setViewportRenderer(core_access.getViewportRenderer());
        this->buildUI();
        this->_running = true;
        this->_uiThread = std::thread(&UserInterface::eventLoop, this);
        //todo check si ca a bien load la font sinon throw
    }

    void UserInterface::destroy()
    {
        this->_running = false;
        if (this->_uiThread.joinable())
            this->_uiThread.join();
        this->_window.setActive(true);
        this->_window.close();
        this->_joinClusterWindow.destroy();
        this->_exploratorWindow.destroy();
        this->_loadWindow.destroy();
    }

    bool UserInterface::shouldExit() const
    {
        return (!this->_running || this->_coreAccess->getState() == ICoreAccess::CoreState::EXITING);
    }

    void UserInterface::exit()
    {
        this->_running = false;
        this->_coreAccess->stop();
    }

    PluginType UserInterface::getType() const
    {
        return (PluginType::USER_INTERFACE);
    }

    extern "C" IPlugin *create_plugin()
    {
        return new UserInterface();
    }

    extern "C" void destroy_plugin(IPlugin *plugin)
    {
        delete plugin;
    }
}
