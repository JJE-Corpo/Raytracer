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
    struct AScreen
    {
        virtual ~AScreen() = default;

        virtual void setFont(sf::Font &font) = 0;
        virtual void handleEvent(sf::RenderWindow &window, sf::Event &event) = 0;

        virtual void prepareFrame() {}

        virtual void shutdown() {}

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
