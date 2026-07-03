//
// Created by jazema on 5/6/26.
//

#ifndef MENUITEM_HPP
#define MENUITEM_HPP
#include <functional>
#include <SFML/Graphics.hpp>

#include "../../Component.hpp"
#include "../../Theme.hpp"

namespace rc
{
    struct MenuItem : Component
    {
        sf::RectangleShape background;
        sf::Text text;

        std::function<void()> onClick;

        bool justClicked = false;

        void setFont(sf::Font &font) override
        {
            text.setFont(font);
        }

        void setLabel(const std::string &str)
        {
            text.setString(str);
            text.setCharacterSize(14);
            text.setFillColor(theme::TEXT_WHITE);
        }

        void layout(float x, float y, float w, float h)
        {
            background.setPosition(x, y);
            background.setSize({w, h});
            text.setPosition(x + 10, y + 4);
        }

        void update(sf::Vector2i mouse) override
        {
            hovered = background.getGlobalBounds().contains((sf::Vector2f)mouse);

            background.setFillColor(
                hovered ? theme::BG_CONTROL_HOVER : theme::BG_ITEM_ALT
            );
            this->justClicked = false;
        }

        void draw(sf::RenderTarget &target, sf::RenderStates states) const override
        {
            target.draw(background, states);
            target.draw(text, states);
        }

        sf::FloatRect getBounds() const override
        {
            return (this->background.getGlobalBounds());
        }

        bool handleEvent(const sf::Event &event, const sf::Vector2i mouse) override
        {
            (void)mouse;
            if (!hovered || !enabled)
                return (false);
            if (event.type == sf::Event::MouseButtonPressed)
            {
                if (onClick)
                {
                    onClick();
                    this->justClicked = true;
                }
                return (true);
            }
            return (false);
        }

        CursorType getCursor() override
        {
            return (this->hovered ? CursorType::HAND : CursorType::ARROW);
        }
    };
}

#endif
