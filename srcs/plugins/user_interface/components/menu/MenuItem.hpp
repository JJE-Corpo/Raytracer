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
        enum class Type
        {
            Action,
            Checkable
        };
        sf::RectangleShape background;
        sf::Text text;

        Type type = Type::Action;

        std::function<void()> onClick;
        std::function<void(bool)> onToggle;

        bool checked = false;
        bool justClicked = false;

        float _x = 0.f, _y = 0.f, _w = 0.f, _h = 24.f;

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
            _x = x;
            _y = y;
            _w = w;
            _h = h;
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

        void draw(sf::RenderWindow &window) override
        {
            window.draw(background);
            window.draw(text);

            if (type == Type::Checkable)
            {
                sf::RectangleShape box;
                box.setSize({14.f, 14.f});
                box.setPosition(_x + _w - 22.f, _y + (_h - 14.f) / 2.f);

                box.setFillColor(checked ? theme::CHECKED
                                         : theme::BG_CONTROL);

                box.setOutlineThickness(1.f);
                box.setOutlineColor(theme::OUTLINE);

                window.draw(box);

                if (checked)
                {
                    sf::Text mark;
                    mark.setFont(*text.getFont());
                    mark.setString("x");
                    mark.setCharacterSize(14);
                    mark.setFillColor(theme::TEXT_WHITE);
                    mark.setPosition(box.getPosition().x + 3.f,
                                     box.getPosition().y - 3.f);

                    window.draw(mark);
                }
            }
        }

        void handleEvent(const sf::Event &event, const sf::Vector2i mouse) override
        {
            (void)mouse;
            if (!hovered || !enabled)
                return;
            if (event.type == sf::Event::MouseButtonPressed)
            {
                if (type == Type::Checkable)
                {
                    checked = !checked;
                    if (onToggle)
                        onToggle(checked);
                }
                if (onClick)
                {
                    onClick();
                    this->justClicked = true;
                }
            }
        }

        CursorType getCursor() override
        {
            return (this->hovered ? CursorType::HAND : CursorType::ARROW);
        }
    };
}

#endif
