//
// Created by jazema on 5/8/26.
//

#ifndef COLORPICKER_HPP
#define COLORPICKER_HPP

#include <algorithm>
#include <functional>
#include <SFML/Graphics.hpp>

#include "../Component.hpp"
#include "Slider.hpp"
#include "../../../common/Color.hpp"
#include "../Theme.hpp"

namespace rc
{
    struct ColorPicker : Component
    {
        mutable sf::RectangleShape swatch;
        mutable sf::RectangleShape popup;
        mutable sf::RectangleShape preview;
        sf::Text label;
        mutable sf::Text title;
        Slider red;
        Slider green;
        Slider blue;

        ColorF color = {1.f, 1.f, 1.f};
        bool open = false;
        bool changed = false;
        std::function<void(const ColorF &)> onChange;

        static constexpr float SWATCH_SIZE = 24.f;
        static constexpr float POPUP_WIDTH = 190.f;
        static constexpr float POPUP_HEIGHT = 130.f;

        void setFont(sf::Font &font) override
        {
            this->label.setFont(font);
            this->label.setCharacterSize(12);
            this->label.setFillColor(theme::TEXT_MAIN);

            this->title.setFont(font);
            this->title.setCharacterSize(12);
            this->title.setFillColor(theme::TEXT_MAIN);

            this->red.setFont(font);
            this->green.setFont(font);
            this->blue.setFont(font);
            this->red.setLabel("R");
            this->green.setLabel("G");
            this->blue.setLabel("B");
            this->red.setRange(0.0f, 255.0f);
            this->green.setRange(0.0f, 255.0f);
            this->blue.setRange(0.0f, 255.0f);
            this->red.labelWidth = 40.0f;
            this->green.labelWidth = 40.0f;
            this->blue.labelWidth = 40.0f;
            this->red.onChange = [this](float value) { this->applyChannel(0, value); };
            this->green.onChange = [this](float value) { this->applyChannel(1, value); };
            this->blue.onChange = [this](float value) { this->applyChannel(2, value); };
            this->syncSlidersFromColor();
        }

        void setLabel(const std::string &text)
        {
            this->label.setString(text);
        }

        void setColor(const ColorF &value)
        {
            this->color = value;
            this->syncSlidersFromColor();
        }

        ColorF getColor() const
        {
            return this->color;
        }

        void openAt(float x, float y)
        {
            this->open = true;
            this->layout(x, y);
        }

        void close()
        {
            this->open = false;
        }

        bool isOpen() const
        {
            return this->open;
        }

        sf::FloatRect getBounds() const override
        {
            return (this->swatch.getGlobalBounds());
        }

        bool contains(sf::Vector2i point) const override
        {
            const float mx = static_cast<float>(point.x);
            const float my = static_cast<float>(point.y);
            if (this->swatch.getGlobalBounds().contains(mx, my))
                return true;
            if (this->open && this->popup.getGlobalBounds().contains(mx, my))
                return true;
            return false;
        }

        int zLayer() const override
        {
            return (this->open ? zlayer::POPUP : zlayer::BASE);
        }

        bool isCapturing() const override
        {
            return (this->open);
        }

        bool processEvent(const sf::Event &event, const sf::Vector2i mouse)
        {
            if (!this->enabled)
                return false;

            if (event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Left)
            {
                if (this->swatch.getGlobalBounds().contains(static_cast<float>(mouse.x), static_cast<float>(mouse.y)))
                {
                    this->open = !this->open;
                    if (this->open)
                        this->layout(this->swatch.getPosition().x - 72.f, this->swatch.getPosition().y);
                    return true;
                }

                if (this->open)
                {
                    if (!this->popup.getGlobalBounds().contains(static_cast<float>(mouse.x), static_cast<float>(mouse.y)))
                    {
                        this->open = false;
                        return true;
                    }

                    this->red.handleEvent(event, mouse);
                    this->green.handleEvent(event, mouse);
                    this->blue.handleEvent(event, mouse);
                    return true;
                }
            }

            if (!this->open)
                return false;

            this->red.handleEvent(event, mouse);
            this->green.handleEvent(event, mouse);
            this->blue.handleEvent(event, mouse);

            if (event.type == sf::Event::KeyPressed && event.key.code == sf::Keyboard::Escape)
            {
                this->open = false;
                return true;
            }

            return true;
        }

        void layout(float x, float y)
        {
            this->label.setPosition({x, y + 4.f});
            this->swatch.setPosition({x + 72.f, y});
            this->swatch.setSize({SWATCH_SIZE, SWATCH_SIZE});

            if (this->open)
            {
                this->popup.setPosition({x, y + 34.f});
                this->popup.setSize({POPUP_WIDTH, POPUP_HEIGHT});

                VerticalLayout layout{x + 10, y + 40, 10.0f};

                this->title.setPosition({layout.x, layout.y});
                layout.next(30);

                this->red.layout(layout.x, layout.y, POPUP_WIDTH - 20.f);
                layout.next(14);
                this->green.layout(layout.x, layout.y, POPUP_WIDTH - 20.f);
                layout.next(14);
                this->blue.layout(layout.x, layout.y, POPUP_WIDTH - 20.f);

                this->preview.setPosition({x + POPUP_WIDTH - 44.f, y + 42.f});
                this->preview.setSize({24.f, 24.f});
            }
        }

        void update(sf::Vector2i mouse) override
        {
            this->hovered = this->swatch.getGlobalBounds().contains(static_cast<float>(mouse.x), static_cast<float>(mouse.y));
            if (!this->open)
                return;

            this->red.update(mouse);
            this->green.update(mouse);
            this->blue.update(mouse);
        }

        bool handleEvent(const sf::Event &event, const sf::Vector2i mouse) override
        {
            return (this->processEvent(event, mouse));
        }

        void draw(sf::RenderTarget &target, sf::RenderStates states) const override
        {
            this->swatch.setFillColor(this->asSfColor());
            this->swatch.setOutlineThickness(1.f);
            this->swatch.setOutlineColor(this->hovered ? theme::OUTLINE_HOVER : theme::OUTLINE_SOFT);
            target.draw(this->label, states);
            target.draw(this->swatch, states);
        }

        // The open pop-up is drawn in the overlay pass so it can extend past a
        // scrolling section without being clipped.
        void drawOverlay(sf::RenderTarget &target, sf::RenderStates states) const override
        {
            if (!this->open)
                return;

            this->popup.setFillColor(theme::BG_POPUP);
            this->popup.setOutlineThickness(1.f);
            this->popup.setOutlineColor(theme::BG_CONTROL_HOVER);
            target.draw(this->popup, states);

            this->title.setString("Color");
            target.draw(this->title, states);

            sf::Color previewColor = this->asSfColor();
            this->preview.setFillColor(previewColor);
            this->preview.setOutlineThickness(1.f);
            this->preview.setOutlineColor(theme::OUTLINE_LIGHT);
            target.draw(this->preview, states);

            target.draw(this->red, states);
            target.draw(this->green, states);
            target.draw(this->blue, states);
        }

        CursorType getCursor() override
        {
            if (!this->enabled)
                return CursorType::ARROW;
            if (this->open && (this->red.dragging || this->green.dragging || this->blue.dragging))
                return CursorType::HAND;
            if (this->hovered)
                return CursorType::HAND;
            return CursorType::ARROW;
        }

    private:
        void applyChannel(int channel, float value)
        {
            value = std::max(0.f, std::min(255.f, value));
            if (channel == 0)
                this->color.r = value / 255.f;
            else if (channel == 1)
                this->color.g = value / 255.f;
            else if (channel == 2)
                this->color.b = value / 255.f;

            this->syncSlidersFromColor();
            this->changed = true;
            if (this->onChange)
                this->onChange(this->color);
        }

        void syncSlidersFromColor()
        {
            this->red.setValue(this->color.r * 255.f);
            this->green.setValue(this->color.g * 255.f);
            this->blue.setValue(this->color.b * 255.f);
        }

        sf::Color asSfColor() const
        {
            return sf::Color(
                static_cast<sf::Uint8>(std::clamp(this->color.r * 255.f, 0.f, 255.f)),
                static_cast<sf::Uint8>(std::clamp(this->color.g * 255.f, 0.f, 255.f)),
                static_cast<sf::Uint8>(std::clamp(this->color.b * 255.f, 0.f, 255.f))
            );
        }
    };
}

#endif