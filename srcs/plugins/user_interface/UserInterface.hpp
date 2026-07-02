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
            std::mutex _windowMutex;
            std::atomic<bool> _running;

            ICoreAccess *_coreAccess;

            // Shared by both screens (see AScreen::setCursorManager) so cursor
            // loading/mapping lives in one place instead of per-screen.
            CursorManager _cursorManager;

            // The two screens UserInterface can drive: the normal editing UI, and
            // the read-only view shown while spectating a cluster render. Which one
            // is "active" is decided purely by the cluster mode - both are pushed
            // through the exact same Screen lifecycle (setFont/handleEvent/
            // prepareFrame/tick/shutdown), so adding a third screen later is just
            // another member plus a branch in activeScreen().
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
