//
// Created by jazema on 4/21/26.
//

#ifndef USERINTERFACE_HPP
#define USERINTERFACE_HPP

#include <SFML/Graphics.hpp>
#include <thread>
#include <mutex>
#include <atomic>

#include "../../common/IPlugin.hpp"
#include "../../common/ISceneRenderer.hpp"
#include "../../common/IUserInterface.hpp"
#include "../../common/ICoreAccess.hpp"
#include "CursorManager.hpp"
#include "screens/ClusterClientScreen.hpp"
#include "screens/DefaultScreen.hpp"
#include "screens/AScreen.hpp"

namespace rc
{
    class UserInterface : public IUserInterface
    {
        private:
            sf::RenderWindow _window;
            sf::Font _font;

            std::thread _uiThread;
            std::atomic<bool> _running;
            bool _destroyed = false;

            ICoreAccess *_coreAccess;

            CursorManager _cursorManager;

            DefaultScreen _defaultScreen;
            ClusterClientScreen _clusterClientScreen;

            AScreen &activeScreen();

            void eventLoop();

            void exit();
            bool shouldExit() const;
        public:
            UserInterface();
            ~UserInterface() override;

            void create(ICoreAccess &core_access) override;
            void destroy() override;

            // IPlugin abstract
            PluginType getType() const override;
    };

}

#endif
