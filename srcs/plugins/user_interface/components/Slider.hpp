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
#include "InlineEditField.hpp"
#include "../../../common/Utils.hpp"

namespace rc
{
    struct Slider : Component
    {
        sf::RectangleShape track;
        sf::RectangleShape fill;
        mutable sf::CircleShape thumb;

        sf::Text label;
        sf::Text valueText;

        float value = 0.5f;
        float minValue = 0.0f;
        float maxValue = 1.0f;

        bool dragging = false;

        float labelWidth = 85.0f;
        float valueWidth = 45.0f;

        std::function<void(float)> onChange;

        InlineEditField editor;

        static constexpr float TRACK_HEIGHT = 4.f;
        static constexpr float THUMB_RADIUS = 7.f;

        static constexpr float HITBOX_HEIGHT = 20.f;

        static constexpr float VALUE_HITBOX_HEIGHT = 18.f;
        static constexpr int VALUE_DOUBLE_CLICK_MS = 400;

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

            this->editor.setFont(font);
            this->editor.setCharacterSize(12);
            this->editor.setValidator([](const std::string &text)
            {
                return (text.empty() || Utils::isFloat(text));
            });
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

        sf::FloatRect trackHitbox() const
        {
            return sf::FloatRect(
                this->track.getPosition().x - THUMB_RADIUS,
                this->track.getPosition().y - HITBOX_HEIGHT * 0.5f,
                this->track.getSize().x + THUMB_RADIUS * 2.f,
                HITBOX_HEIGHT
            );
        }

        sf::FloatRect valueBounds() const
        {
            const sf::Vector2f pos = this->valueText.getPosition();
            return sf::FloatRect(pos.x, pos.y, this->valueWidth, VALUE_HITBOX_HEIGHT);
        }

        sf::FloatRect getBounds() const override
        {
            const sf::FloatRect track = this->trackHitbox();
            const sf::FloatRect value = this->valueBounds();
            const float left = std::min(track.left, value.left);
            const float top = std::min(track.top, value.top);
            const float right = std::max(track.left + track.width, value.left + value.width);
            const float bottom = std::max(track.top + track.height, value.top + value.height);
            return sf::FloatRect(left, top, right - left, bottom - top);
        }

        bool isCapturing() const override
        {
            return (this->dragging || this->editor.isCapturing());
        }

        void update(sf::Vector2i mouse) override
        {
            if (this->editor.active)
            {
                this->editor.update(mouse);
                return;
            }

            this->hovered = this->trackHitbox().contains(
                static_cast<float>(mouse.x),
                static_cast<float>(mouse.y)
            );
            this->valueHovered = this->valueBounds().contains(
                static_cast<float>(mouse.x),
                static_cast<float>(mouse.y)
            );

            if (!this->dragging)
                return;

            this->setValueFromMouse(mouse.x);
        }

        bool handleEvent(
            const sf::Event &event,
            const sf::Vector2i mouse
        ) override
        {
            if (!this->enabled)
                return (false);

            if (this->editor.active)
                return (this->editor.handleEvent(event, mouse));

            if (
                event.type == sf::Event::MouseButtonPressed &&
                event.mouseButton.button == sf::Mouse::Left
            )
            {
                if (this->valueBounds().contains(static_cast<float>(mouse.x), static_cast<float>(mouse.y)))
                {
                    const bool is_double_click = this->valuePendingClick
                        && this->valueClickClock.getElapsedTime().asMilliseconds() < VALUE_DOUBLE_CLICK_MS;
                    if (is_double_click)
                    {
                        this->valuePendingClick = false;
                        this->beginEdit();
                    }
                    else
                    {
                        this->valuePendingClick = true;
                        this->valueClickClock.restart();
                    }
                    return (true);
                }

                if (this->trackHitbox().contains(static_cast<float>(mouse.x), static_cast<float>(mouse.y)))
                {
                    this->dragging = true;
                    this->setValueFromMouse(mouse.x);
                    return (true);
                }
                return (false);
            }

            if (
                event.type == sf::Event::MouseButtonReleased &&
                event.mouseButton.button == sf::Mouse::Left
            )
            {
                const bool wasDragging = this->dragging;
                this->dragging = false;
                return (wasDragging);
            }
            return (false);
        }

        void draw(sf::RenderTarget &target, sf::RenderStates states) const override
        {
            this->thumb.setFillColor(this->dragging || this->hovered ? HOVER_COLOR : THUMB_COLOR);

            target.draw(this->track, states);
            target.draw(this->fill, states);
            target.draw(this->thumb, states);

            target.draw(this->label, states);
            if (this->editor.active)
                target.draw(this->editor, states);
            else
                target.draw(this->valueText, states);
        }

        CursorType getCursor() override
        {
            if (!this->enabled)
                return (CursorType::NOT_ALLOWED);
            if (this->editor.active)
                return (this->editor.getCursor());
            if (this->dragging || this->hovered)
                return (CursorType::HAND);
            if (this->valueHovered)
                return (CursorType::TEXT);
            return (CursorType::ARROW);
        }

    private:

        bool valueHovered = false;
        bool valuePendingClick = false;
        sf::Clock valueClickClock;

        void beginEdit()
        {
            char buffer[32];
            std::snprintf(buffer, sizeof(buffer), "%.2f", this->value);

            this->editor.onCommit = [this](const std::string &text)
            {
                this->applyEditedValue(text);
            };

            const sf::FloatRect bounds = this->valueBounds();
            this->editor.begin(buffer, bounds.left, bounds.top, bounds.width, bounds.height);
        }

        void applyEditedValue(const std::string &text)
        {
            if (!Utils::isFloat(text))
                return;
            this->setValue(std::stof(text));
            if (this->onChange)
                this->onChange(this->value);
        }

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