//
// Created by jazema on 5/16/26.
//

#ifndef SCREEN_HPP
#define SCREEN_HPP
#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/Window/Event.hpp>
#include <SFML/Window/Window.hpp>

#include "../CursorManager.hpp"
#include "../Theme.hpp"

namespace rc
{
    // A top-level UI mode that owns the whole window: the normal editing UI
    // (DefaultScreen) or the read-only cluster spectate view (ClusterClientScreen).
    // UserInterface drives whichever one is active through this exact lifecycle.
    struct AScreen
    {
        virtual ~AScreen() = default;

        virtual void setFont(sf::Font &font) = 0;
        virtual void handleEvent(sf::RenderWindow &window, sf::Event &event) = 0;

        // Called once per frame before the window is locked/cleared, so a screen
        // can do expensive off-screen work (e.g. rendering the scene into its own
        // buffer) without holding up other threads waiting on the window.
        virtual void prepareFrame() {}

        // Called once when the screen is being torn down (UserInterface::destroy),
        // so a screen can close any sub-windows/threads it owns.
        virtual void shutdown() {}

        // Wired once by UserInterface after construction, so every screen shares
        // the same set of already-loaded system cursors instead of each screen
        // loading/owning its own (DefaultScreen used to do this on its own).
        void setCursorManager(CursorManager &cursorManager)
        {
            this->_cursorManager = &cursorManager;
        }

        void tick(sf::RenderWindow &window)
        {
            window.clear(theme::BG_WINDOW);
            this->update(window);
            this->draw(window);
            window.display();
        }

        protected:
            virtual void update(sf::RenderWindow &window) = 0;
            virtual void draw(sf::RenderWindow &window) = 0;

            void applyCursor(sf::RenderWindow &window, CursorType type) const
            {
                if (this->_cursorManager)
                    this->_cursorManager->apply(window, type);
            }

        private:
            CursorManager *_cursorManager = nullptr;
    };
}

#endif
