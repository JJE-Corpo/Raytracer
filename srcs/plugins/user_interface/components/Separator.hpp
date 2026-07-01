//
// Created by jazema on 5/1/26.
//

#ifndef SEPARATOR_HPP
#define SEPARATOR_HPP

#include <SFML/Graphics.hpp>

#include "../Component.hpp"
#include "../Theme.hpp"

namespace rc
{
    struct Separator : Component
    {
        sf::RectangleShape line;

        void layout(float x, float y, float width)
        {
            this->line.setPosition({x, y});
            this->line.setSize({width, 1.f});
            this->line.setFillColor(theme::BG_CONTROL);
        }

        void draw(sf::RenderTarget &target, sf::RenderStates states) const override
        {
            target.draw(this->line, states);
        }
    };
}

#endif
