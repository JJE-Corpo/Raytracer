//
// Created by jazema on 5/5/26.
//

#ifndef COMPONENT_HPP
#define COMPONENT_HPP

#include <SFML/Graphics.hpp>

#include "CursorType.hpp"

namespace rc
{
    struct Component : public sf::Drawable
    {
        bool enabled = true;
        bool hovered = false;
        bool visible = true;

        virtual ~Component() = default;

        void draw(sf::RenderTarget &target, sf::RenderStates states) const override = 0;
        virtual void update(sf::Vector2i mouse)
        {
            (void)mouse;
        }
        virtual void handleEvent(const sf::Event &event, const sf::Vector2i mouse)
        {
            (void)event;
            (void)mouse;
        }
        virtual void setFont(sf::Font &font)
        {
            (void)font;
        }

        virtual CursorType getCursor()
        {
            return (CursorType::ARROW);
        }
    };
}

#endif
