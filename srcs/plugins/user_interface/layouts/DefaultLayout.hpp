//
// Created by jazema on 5/16/26.
//

#ifndef DEFAULTLAYOUT_HPP
#define DEFAULTLAYOUT_HPP
#include "Layout.hpp"

namespace rc
{

    struct DefaultLayout: Layout
    {
        void setFont(sf::Font &font) override
        {
            (void)font;
        }
        void handleEvent(sf::RenderWindow &window, sf::Event &event) override
        {
            (void)window;
            (void)event;
        }

        protected:
            void update(sf::RenderWindow &window) override
            {
                (void)window;
            }
            void draw(sf::RenderWindow &window) override
            {
                (void)window;
            }
    };
}

#endif
