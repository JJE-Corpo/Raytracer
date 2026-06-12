//
// Created by jazema on 5/8/26.
//

#ifndef SLIDER_HPP
#define SLIDER_HPP

#include <functional>
#include <SFML/Graphics.hpp>
#include <algorithm>
#include <cstdio>
#include <cmath>

#include "../Component.hpp"
#include "../Theme.hpp"

namespace rc
{
    struct Slider : Component
    {
        sf::RectangleShape track;
        sf::RectangleShape fill;
        sf::CircleShape thumb;

        sf::Text label;
        sf::Text valueText;

        float value = 0.5f;
        float minValue = 0.0f;
        float maxValue = 1.0f;

        bool dragging = false;

        float labelWidth = 85.0f;
        float valueWidth = 45.0f;

        std::function<void(float)> onChange;

        static constexpr float TRACK_HEIGHT = 4.f;
        static constexpr float THUMB_RADIUS = 7.f;

        static constexpr float HITBOX_HEIGHT = 20.f;

        const sf::Color TRACK_COLOR = theme::TRACK;
        const sf::Color FILL_COLOR = theme::ACCENT;
        const sf::Color THUMB_COLOR = theme::THUMB;
        const sf::Color HOVER_COLOR = theme::TEXT_WHITE;

        void setFont(sf::Font &font) override
        {
            this->label.setFont(font);
            this->label.setCharacterSize(12);
            this->label.setFillColor(theme::TEXT_MAIN);

            this->valueText.setFont(font);
            this->valueText.setCharacterSize(12);
            this->valueText.setFillColor(theme::TEXT_DIM);
        }

        void setLabel(const std::string &text)
        {
            this->label.setString(text);
        }

        void setValue(float v)
        {
            this->value = std::clamp(v, this->minValue, this->maxValue);

            this->updateValueText();
            this->updateVisuals();
        }

        float getValue() const
        {
            return this->value;
        }

        void setRange(float min_val, float max_val)
        {
            this->minValue = min_val;
            this->maxValue = max_val;

            this->setValue(this->value);
        }

        void layout(float x, float y, float width)
        {
            this->label.setPosition({x, y});

            float slider_x = x + this->labelWidth;

            float slider_width = width - this->labelWidth - this->valueWidth;

            slider_width -= THUMB_RADIUS * 2.f;

            slider_x += THUMB_RADIUS;

            this->track.setPosition({slider_x, y + 9.f});
            this->track.setSize({slider_width, TRACK_HEIGHT});
            this->track.setFillColor(TRACK_COLOR);

            this->fill.setFillColor(FILL_COLOR);

            this->thumb.setRadius(THUMB_RADIUS);
            this->thumb.setOrigin({THUMB_RADIUS, THUMB_RADIUS});

            this->valueText.setPosition({slider_x + slider_width + THUMB_RADIUS * 2, y});

            this->updateVisuals();
        }

        void update(sf::Vector2i mouse) override
        {
            sf::FloatRect hover_bounds(
                this->track.getPosition().x - THUMB_RADIUS,
                this->track.getPosition().y - HITBOX_HEIGHT * 0.5f,
                this->track.getSize().x + THUMB_RADIUS * 2.f,
                HITBOX_HEIGHT
            );

            this->hovered = hover_bounds.contains(
                static_cast<float>(mouse.x),
                static_cast<float>(mouse.y)
            );

            if (!this->dragging)
                return;

            this->setValueFromMouse(mouse.x);
        }

        void handleEvent(
            const sf::Event &event,
            const sf::Vector2i mouse
        ) override
        {
            if (!this->enabled)
                return;

            if (
                event.type == sf::Event::MouseButtonPressed &&
                event.mouseButton.button == sf::Mouse::Left
            )
            {
                sf::FloatRect click_bounds(
                    this->track.getPosition().x - THUMB_RADIUS,
                    this->track.getPosition().y - HITBOX_HEIGHT * 0.5f,
                    this->track.getSize().x + THUMB_RADIUS * 2.f,
                    HITBOX_HEIGHT
                );

                if (
                    click_bounds.contains(
                        static_cast<float>(mouse.x),
                        static_cast<float>(mouse.y)
                    )
                )
                {
                    this->dragging = true;
                    this->setValueFromMouse(mouse.x);
                }
            }

            if (
                event.type == sf::Event::MouseButtonReleased &&
                event.mouseButton.button == sf::Mouse::Left
            )
            {
                this->dragging = false;
            }
        }

        void draw(sf::RenderWindow &w) override
        {
            this->thumb.setFillColor(this->dragging || this->hovered ? HOVER_COLOR : THUMB_COLOR);

            w.draw(this->track);
            w.draw(this->fill);
            w.draw(this->thumb);

            w.draw(this->label);
            w.draw(this->valueText);
        }

        CursorType getCursor() override
        {
            if (!this->enabled)
                return (CursorType::NOT_ALLOWED);
            if (this->dragging || this->hovered)
                return (CursorType::HAND);
            return (CursorType::ARROW);
        }

    private:

        float normalizedValue() const
        {
            if (this->maxValue - this->minValue == 0.0f)
                return (0.0f);
            return (this->value - this->minValue) / (this->maxValue - this->minValue);
        }

        void setValueFromMouse(int mouse_x)
        {
            float sx = this->track.getPosition().x;
            float sw = this->track.getSize().x;

            float normalized = (static_cast<float>(mouse_x) - sx) / sw;

            normalized = std::clamp(normalized, 0.0f, 1.0f);

            float new_value = this->minValue + normalized * (this->maxValue - this->minValue);

            if (std::fabs(new_value - this->value) > 1e-6f)
            {
                this->value = new_value;

                this->updateValueText();
                this->updateVisuals();

                if (this->onChange)
                    this->onChange(this->value);
            }
        }

        void updateVisuals()
        {
            float n = this->normalizedValue();

            float sx = this->track.getPosition().x;
            float sy = this->track.getPosition().y;
            float sw = this->track.getSize().x;

            float thumb_x = sx + sw * n;

            this->fill.setPosition(this->track.getPosition());
            this->fill.setSize({thumb_x - sx, TRACK_HEIGHT});

            this->thumb.setPosition({thumb_x, sy + TRACK_HEIGHT * 0.5f});
        }

        void updateValueText()
        {
            char buffer[32];

            std::snprintf(buffer, sizeof(buffer), "%.2f", this->value);
            this->valueText.setString(buffer);
        }
    };
}

#endif