//
// Created by jazema on 5/1/26.
//

#ifndef BUTTON_HPP
#define BUTTON_HPP

#include <functional>
#include <SFML/Graphics.hpp>

#include "../Component.hpp"
#include "../Theme.hpp"

namespace rc
{
    struct Button : Component
    {
        mutable sf::RectangleShape shape;
        sf::Text label;

        bool pressed = false;

        std::function<void()> onClick;

        void setFont(sf::Font &font) override
        {
            this->label.setFont(font);
            this->label.setCharacterSize(16);
            this->label.setFillColor(theme::TEXT_WHITE);
        }

        void setLabel(const std::string &text)
        {
            this->label.setString(text);
        }

        void layout(float x, float y, float w, float h)
        {
            this->shape.setPosition({x, y});
            this->shape.setSize({w, h});

            sf::FloatRect bounds = this->label.getLocalBounds();
            this->label.setPosition({
                x + (w - bounds.width) / 2.f,
                y + (h - bounds.height) / 2.f - 4.f
            });
        }

        sf::FloatRect getBounds() const override
        {
            return (this->shape.getGlobalBounds());
        }

        void update(const sf::Vector2i mouse) override
        {
            this->hovered = this->shape.getGlobalBounds().contains((float)mouse.x, (float)mouse.y);
        }

        bool handleEvent(const sf::Event &event, const sf::Vector2i mouse) override
        {
            if (!this->enabled)
                return (false);
            const bool inside = shape.getGlobalBounds().contains((float)mouse.x, (float)mouse.y);
            if (event.type == sf::Event::MouseButtonPressed && inside)
            {
                this->pressed = true;
                return (true);
            }
            if (event.type == sf::Event::MouseButtonReleased)
            {
                const bool consumed = this->pressed && inside;
                if (consumed && this->onClick)
                    this->onClick();
                this->pressed = false;
                return (consumed);
            }
            return (false);
        }

        CursorType getCursor() override
        {
            if (!this->enabled && this->hovered)
                return (CursorType::NOT_ALLOWED);
            if (this->hovered)
                return (CursorType::HAND);
            return (CursorType::ARROW);
        }

        void draw(sf::RenderTarget &target, sf::RenderStates states) const override
        {
            if (!enabled)
                this->shape.setFillColor(theme::BG_CONTROL_HOVER);
            else if (pressed)
                this->shape.setFillColor(theme::BUTTON_PRESSED);
            else if (hovered)
                this->shape.setFillColor(theme::BG_CONTROL);
            else
                this->shape.setFillColor(theme::BG_ITEM);
            target.draw(this->shape, states);
            target.draw(this->label, states);
        }
    };
}

#endif
