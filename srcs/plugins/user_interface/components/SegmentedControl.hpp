//
// Created by jazema on 5/9/26.
//

#ifndef SEGMENTEDCONTROL_HPP
#define SEGMENTEDCONTROL_HPP

#include <functional>
#include <SFML/Graphics.hpp>

#include "../Component.hpp"
#include "../Theme.hpp"

namespace rc
{
    struct SegmentedControl : Component
    {
        sf::Text label;
        sf::RectangleShape background;
        mutable sf::RectangleShape leftButton;
        mutable sf::RectangleShape rightButton;
        sf::Text leftText;
        sf::Text rightText;

        int selectedIndex = 0;
        bool leftHovered = false;
        bool rightHovered = false;
        std::function<void(int)> onChange;

        static constexpr float LABEL_WIDTH = 54.f;
        static constexpr float CONTROL_HEIGHT = 22.f;

        void setFont(sf::Font &font) override
        {
            this->label.setFont(font);
            this->label.setCharacterSize(12);
            this->label.setFillColor(theme::TEXT_WHITE);

            this->leftText.setFont(font);
            this->leftText.setCharacterSize(12);
            this->leftText.setFillColor(theme::TEXT_WHITE);

            this->rightText.setFont(font);
            this->rightText.setCharacterSize(12);
            this->rightText.setFillColor(theme::TEXT_WHITE);
        }

        void setLabel(const std::string &text)
        {
            this->label.setString(text);
        }

        void setOptions(const std::string &left, const std::string &right)
        {
            this->leftText.setString(left);
            this->rightText.setString(right);
        }

        void setSelectedIndex(int index)
        {
            this->selectedIndex = (index <= 0 ? 0 : 1);
        }

        void layout(float x, float y, float width)
        {
            this->label.setPosition({x, y + 2.f});

            float control_x = x + LABEL_WIDTH + 12.f;
            float control_width = std::max(120.f, width - LABEL_WIDTH - 12.f);
            float half_width = control_width * 0.5f;

            this->background.setPosition({control_x, y});
            this->background.setSize({control_width, CONTROL_HEIGHT});
            this->background.setFillColor(theme::BG_CONTROL);
            this->background.setOutlineThickness(1.f);
            this->background.setOutlineColor(theme::OUTLINE);

            this->leftButton.setPosition({control_x, y});
            this->leftButton.setSize({half_width, CONTROL_HEIGHT});

            this->rightButton.setPosition({control_x + half_width, y});
            this->rightButton.setSize({control_width - half_width, CONTROL_HEIGHT});

            sf::FloatRect leftBounds = this->leftText.getLocalBounds();
            sf::FloatRect rightBounds = this->rightText.getLocalBounds();

            this->leftText.setPosition({
                control_x + (half_width - leftBounds.width) / 2.f,
                y + 2.f
            });

            this->rightText.setPosition({
                control_x + half_width + ((control_width - half_width) - rightBounds.width) / 2.f,
                y + 2.f
            });
        }

        void update(sf::Vector2i mouse) override
        {
            this->leftHovered = this->leftButton.getGlobalBounds().contains((float)mouse.x, (float)mouse.y);
            this->rightHovered = this->rightButton.getGlobalBounds().contains((float)mouse.x, (float)mouse.y);
            this->hovered = this->leftHovered || this->rightHovered;
        }

        sf::FloatRect getBounds() const override
        {
            return (this->background.getGlobalBounds());
        }

        bool handleEvent(const sf::Event &event, const sf::Vector2i mouse) override
        {
            if (!this->enabled)
                return (false);

            if (event.type != sf::Event::MouseButtonPressed || event.mouseButton.button != sf::Mouse::Left)
                return (false);

            if (this->leftButton.getGlobalBounds().contains((float)mouse.x, (float)mouse.y))
            {
                this->selectedIndex = 0;
                if (this->onChange)
                    this->onChange(this->selectedIndex);
                return (true);
            }
            if (this->rightButton.getGlobalBounds().contains((float)mouse.x, (float)mouse.y))
            {
                this->selectedIndex = 1;
                if (this->onChange)
                    this->onChange(this->selectedIndex);
                return (true);
            }
            return (false);
        }

        void draw(sf::RenderTarget &target, sf::RenderStates states) const override
        {
            sf::Color leftColor = (this->selectedIndex == 0) ? theme::BUTTON_PRESSED : theme::BG_ITEM;
            sf::Color rightColor = (this->selectedIndex == 1) ? theme::BUTTON_PRESSED : theme::BG_ITEM;

            if (!this->enabled)
            {
                leftColor = theme::BG_CONTROL_HOVER;
                rightColor = theme::BG_CONTROL_HOVER;
            }
            else
            {
                if (this->leftHovered)
                    leftColor = theme::BG_CONTROL;
                if (this->rightHovered)
                    rightColor = theme::BG_CONTROL;
            }

            this->leftButton.setFillColor(leftColor);
            this->rightButton.setFillColor(rightColor);

            target.draw(this->label, states);
            target.draw(this->background, states);
            target.draw(this->leftButton, states);
            target.draw(this->rightButton, states);
            target.draw(this->leftText, states);
            target.draw(this->rightText, states);
        }

        CursorType getCursor() override
        {
            if (!this->enabled)
                return CursorType::ARROW;
            return this->hovered ? CursorType::HAND : CursorType::ARROW;
        }
    };
}

#endif