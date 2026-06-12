//
// Created by jazema on 5/16/26.
//

#ifndef LAYOUT_HPP
#define LAYOUT_HPP
#include <functional>
#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/Window/Event.hpp>
#include <SFML/Window/Window.hpp>

#include "../Theme.hpp"

namespace rc
{
    struct Layout
    {
        virtual               ~Layout() = default;

        std::function<void()> exit       = [] {};
        std::function<bool()> shouldExit = [] { return (false); };

        virtual void setFont(sf::Font &font) = 0;
        virtual void handleEvent(sf::RenderWindow &window, sf::Event &event) = 0;

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
    };
}

#endif
