//
// Created by jazema on 5/1/26.
//

#ifndef CHECKBOX_HPP
#define CHECKBOX_HPP
#include <functional>
#include <SFML/Graphics.hpp>

#include "../Component.hpp"
#include "../Theme.hpp"

namespace rc
{
    struct Checkbox : Component
    {
        mutable sf::RectangleShape box;
        sf::Text label;

        bool checked = false;
        std::function<void(bool)> onToggle;

        static constexpr float BOX_SIZE = 18.f;
        static constexpr float PADDING_X = 10.f;

        sf::FloatRect getBounds() const override
        {
            sf::FloatRect b = this->box.getGlobalBounds();
            sf::FloatRect l = this->label.getGlobalBounds();

            float left = std::min(b.left, l.left);
            float top = std::min(b.top, l.top);
            float right = std::max(b.left + b.width,  l.left + l.width);
            float bottom = std::max(b.top  + b.height, l.top  + l.height);

            return sf::FloatRect(left, top, right - left, bottom - top);
        }

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

        void setChecked(bool v)
        {
            this->checked = v;
            this->box.setFillColor(this->checked ? theme::CHECKED : theme::BG_CONTROL);
        }

        void layout(float x, float y)
        {
            this->box.setSize({BOX_SIZE, BOX_SIZE});
            this->box.setPosition({x, y});

            float textY = y + (BOX_SIZE - this->label.getCharacterSize()) / 2.f - 2.f;
            this->label.setPosition({x + BOX_SIZE + PADDING_X, textY});
        }

        void update(sf::Vector2i mouse) override
        {
            this->hovered = getBounds().contains((float)mouse.x, (float)mouse.y);
        }

        bool handleEvent(const sf::Event &event, const sf::Vector2i mouse) override
        {
            (void)mouse;
            if (event.type == sf::Event::MouseButtonPressed && this->hovered)
            {
                this->checked = !this->checked;
                if (this->onToggle)
                    this->onToggle(this->checked);
                return (true);
            }
            return (false);
        }

        void draw(sf::RenderTarget &target, sf::RenderStates states) const override
        {
            if (this->checked)
                this->box.setFillColor(theme::CHECKED);
            else if (hovered)
                this->box.setFillColor(theme::OUTLINE_MID);
            else
                this->box.setFillColor(theme::BG_CONTROL);

            target.draw(this->box, states);
            target.draw(this->label, states);
        }
    };
}

#endif
