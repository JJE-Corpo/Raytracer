//
// Created by jazema on 4/21/26.
//

#include "UserInterface.hpp"
#include "../../common/IPlugin.hpp"
#include "../../common/ICoreAccess.hpp"
#include "../../common/cluster/IClusterClient.hpp"
#include "Theme.hpp"

namespace rc
{
    UserInterface::UserInterface() : _running(false), _coreAccess(nullptr), _clusterClientScreen()
    {
    }

    UserInterface::~UserInterface()
    {
        this->UserInterface::destroy();
    }

    Screen &UserInterface::activeScreen()
    {
        if (this->_coreAccess->getClusterModule()->getClusterMode() == ClusterMode::CLIENT)
            return (this->_clusterClientScreen);
        return (this->_defaultScreen);
    }

    void UserInterface::eventLoop()
    {
        {
            std::lock_guard lock(this->_windowMutex);
            this->_window.setActive(true);
            this->_window.setFramerateLimit(60);
        }
        while (!this->shouldExit())
        {
            Screen &screen = this->activeScreen();

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
            if (this->shouldExit())
                break;

            // The (potentially expensive) scene render happens outside the window
            // lock, so it never blocks anything else waiting on the window.
            screen.prepareFrame();
            {
                std::lock_guard lock(this->_windowMutex);
                screen.tick(this->_window);
            }
        }
        this->_window.setActive(false);
    }

    void UserInterface::create(ICoreAccess &core_access)
    {
        this->_coreAccess = &core_access;
        if (!this->_font.loadFromFile("assets/font.ttf"))
            throw (std::runtime_error("Failed to load font"));

        this->_window.create({1280, 720}, "Raytracer UI grrahboom", sf::Style::Default);
        this->_window.setActive(false);

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
