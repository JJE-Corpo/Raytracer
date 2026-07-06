//
// Created by jazema on 4/21/26.
//

#include "UserInterface.hpp"
#include "../../common/IPlugin.hpp"
#include "../../common/ICoreAccess.hpp"
#include "../../common/cluster/IClusterClient.hpp"
#include "Theme.hpp"

#include <cctype>
#include <chrono>
#include <cstdlib>
#include <fstream>
#include <string>

#include "windows/Window.hpp"

// Make Xlib thread-safe (declared here to avoid pulling in <X11/Xlib.h>, whose
// macros clash with SFML's enums). Linked via -lX11.
extern "C" int XInitThreads();

namespace rc
{
    namespace
    {
        // True when running under WSL, where WSLg's hardware OpenGL crashes with a
        // second GL context (see UserInterface::create).
        bool runningUnderWsl()
        {
            if (const char *distro = std::getenv("WSL_DISTRO_NAME"); distro && distro[0] != '\0')
                return (true);
            std::ifstream version("/proc/version");
            std::string line;
            if (version && std::getline(version, line))
            {
                for (char &c : line)
                    c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
                return (line.find("microsoft") != std::string::npos || line.find("wsl") != std::string::npos);
            }
            return (false);
        }
    }

    UserInterface::UserInterface() : _running(false), _coreAccess(nullptr)
    {
    }

    UserInterface::~UserInterface()
    {
        this->UserInterface::destroy();
    }

    AScreen &UserInterface::activeScreen()
    {
        if (this->_coreAccess->getClusterModule()->getClusterMode() == ClusterMode::CLIENT)
            return (this->_clusterClientScreen);
        return (this->_defaultScreen);
    }

    void UserInterface::eventLoop()
    {
        {
            std::lock_guard<std::mutex> lock(gWindowXLock());
            this->_window.setActive(true);
        }
        while (!this->shouldExit())
        {
            AScreen &screen = this->activeScreen();

            {
                // Event polling reads from the shared X connection, so it takes
                // the same lock the pop-up windows use (see gWindowXLock).
                std::lock_guard<std::mutex> lock(gWindowXLock());
                sf::Event event;
                while (this->_window.pollEvent(event) && !this->shouldExit())
                {
                    if (event.type == sf::Event::Closed)
                    {
                        this->_running = false;
                        this->_coreAccess->stop();
                        break;
                    }
                    if (event.type == sf::Event::Resized)
                    {
                        sf::FloatRect visibleArea(0, 0, static_cast<float>(event.size.width), static_cast<float>(event.size.height));
                        this->_window.setView(sf::View(visibleArea));
                    }
                    screen.handleEvent(this->_window, event);
                }
            }
            if (this->shouldExit())
                break;

            // The (potentially expensive) scene render happens outside the window
            // lock, so it never blocks anything else waiting on the window.
            screen.prepareFrame();
            {
                std::lock_guard<std::mutex> lock(gWindowXLock());
                screen.tick(this->_window);
            }
            // Pace the UI (~60 FPS) with the X lock released; replaces the window
            // framerate limit, whose internal sleep would otherwise hold the
            // shared lock and starve the pop-up windows.
            std::this_thread::sleep_for(std::chrono::milliseconds(12));
        }
        {
            std::lock_guard<std::mutex> lock(gWindowXLock());
            this->_window.setActive(false);
        }
    }

    void UserInterface::create(ICoreAccess &core_access)
    {
        // The UI event loop and every pop-up window run on their own threads and
        // all talk to X, so Xlib must be made thread-safe before any window is
        // created; without it WSLg aborts with an xcb assertion (native Linux
        // only tolerated it by timing luck).
        XInitThreads();
        // WSLg's hardware OpenGL (d3d12-backed Mesa) segfaults as soon as a second
        // GL context renders textures, i.e. whenever a pop-up window opens.
        // Software rendering (llvmpipe) handles multiple contexts fine, so force
        // it ONLY under WSL - native Linux keeps fast hardware GL and is untouched.
        // Must happen before the first window/context is created.
        if (runningUnderWsl())
            setenv("LIBGL_ALWAYS_SOFTWARE", "1", 1);

        this->_coreAccess = &core_access;
        if (!this->_font.loadFromFile("assets/font.ttf"))
            throw (std::runtime_error("Failed to load font"));

        this->_window.create({1280, 720}, "Raytracer UI grrahboom", sf::Style::Default);
        this->_window.setActive(false);

        this->_cursorManager.load();
        this->_defaultScreen.setCursorManager(this->_cursorManager);
        this->_clusterClientScreen.setCursorManager(this->_cursorManager);

        this->_defaultScreen.setCoreAccess(this->_coreAccess);
        this->_defaultScreen.onClusterJoined = [this](IClusterClient *client)
        {
            this->_clusterClientScreen.setClient(client);
        };

        this->_clusterClientScreen.setFont(this->_font);
        this->_defaultScreen.setFont(this->_font);

        this->_clusterClientScreen.setOnLeave([this]
        {
            try
            {
                this->_coreAccess->getClusterModule()->leaveCluster();
                this->_clusterClientScreen.setClient(nullptr);
            }
            catch (std::exception &e)
            {
                this->_clusterClientScreen.getToastManager().push("Cannot leave cluster", e.what(), ToastType::ERROR);
            }
        });
        this->_clusterClientScreen.setViewportRenderer(core_access.getViewportRenderer());

        this->_defaultScreen.buildUI();
        this->_running = true;
        this->_uiThread = std::thread(&UserInterface::eventLoop, this);
    }

    void UserInterface::destroy()
    {
        this->_running = false;
        if (this->_uiThread.joinable())
            this->_uiThread.join();
        this->_window.setActive(true);
        this->_window.close();
        this->_defaultScreen.shutdown();
        this->_clusterClientScreen.shutdown();
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
