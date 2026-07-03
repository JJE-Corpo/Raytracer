//
// Created by jazema on 5/8/26.
//

#ifndef COLORPICKER_HPP
#define COLORPICKER_HPP

#include <algorithm>
#include <cstdio>
#include <functional>
#include <string>
#include <SFML/Graphics.hpp>

#include "../Component.hpp"
#include "../LayoutPen.hpp"
#include "Slider.hpp"
#include "TextField.hpp"
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
        sf::Text hexLabel;
        TextField hexField;

        ColorF color = {1.f, 1.f, 1.f};
        bool open = false;
        bool changed = false;
        std::function<void(const ColorF &)> onChange;

        static constexpr float SWATCH_SIZE = 24.f;
        static constexpr float POPUP_WIDTH = 190.f;
        static constexpr float POPUP_HEIGHT = 150.f;
        static constexpr float HEX_LABEL_WIDTH = 40.f;
        static constexpr float HEX_FIELD_HEIGHT = 20.f;

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

            this->hexLabel.setFont(font);
            this->hexLabel.setCharacterSize(12);
            this->hexLabel.setFillColor(theme::TEXT_MAIN);
            this->hexLabel.setString("Hex");

            this->hexField.setFont(font);
            this->hexField.setCharacterSize(12);
            // Accept only an optional leading '#' followed by up to six hex
            // digits, so the field can never hold a string that is not a prefix
            // of a valid colour.
            this->hexField.onType = [](const std::string &text)
            {
                return (ColorPicker::isHexInput(text));
            };
            // Enter just drops focus; the colour is already applied live as the
            // sixth digit is typed (see tryApplyHex).
            this->hexField.onValidate = [this](const std::string &)
            {
                this->tryApplyHex();
                return (true);
            };

            this->syncFromColor();
        }

        void setLabel(const std::string &text)
        {
            this->label.setString(text);
        }

        void setColor(const ColorF &value)
        {
            this->color = value;
            this->syncFromColor();
        }

        ColorF getColor() const
        {
            return this->color;
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
                    if (this->open)
                        this->close();
                    else
                    {
                        this->open = true;
                        this->layout(this->swatch.getPosition().x - 72.f, this->swatch.getPosition().y);
                    }
                    return true;
                }

                if (this->open)
                {
                    if (!this->popup.getGlobalBounds().contains(static_cast<float>(mouse.x), static_cast<float>(mouse.y)))
                    {
                        this->close();
                        return true;
                    }

                    // The hex field is routed first so a click inside it takes
                    // focus and a click elsewhere in the popup drops that focus.
                    this->hexField.handleEvent(event, mouse);
                    this->red.handleEvent(event, mouse);
                    this->green.handleEvent(event, mouse);
                    this->blue.handleEvent(event, mouse);
                    return true;
                }
            }

            if (!this->open)
                return false;

            if (event.type == sf::Event::KeyPressed && event.key.code == sf::Keyboard::Escape)
            {
                this->close();
                return true;
            }

            // Typing (and the caret/selection keys) is applied live: tryApplyHex
            // updates the colour as soon as the field holds a complete hex code.
            if (this->hexField.handleEvent(event, mouse))
            {
                this->tryApplyHex();
                return true;
            }

            this->red.handleEvent(event, mouse);
            this->green.handleEvent(event, mouse);
            this->blue.handleEvent(event, mouse);

            return true;
        }

        // Close the popup, dropping keyboard focus from the hex field and
        // reformatting its text to the canonical "#RRGGBB" of the final colour.
        void close()
        {
            this->open = false;
            this->hexField.focused = false;
            this->syncFromColor();
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

                LayoutPen layout{x + 10, y + 40, 10.0f};

                this->title.setPosition({layout.x, layout.y});
                layout.next(30);

                this->red.layout(layout.x, layout.y, POPUP_WIDTH - 20.f);
                layout.next(14);
                this->green.layout(layout.x, layout.y, POPUP_WIDTH - 20.f);
                layout.next(14);
                this->blue.layout(layout.x, layout.y, POPUP_WIDTH - 20.f);
                layout.next(14);

                this->hexLabel.setPosition({layout.x, layout.y + 3.f});
                this->hexField.layout(
                    layout.x + HEX_LABEL_WIDTH,
                    layout.y,
                    POPUP_WIDTH - 20.f - HEX_LABEL_WIDTH,
                    HEX_FIELD_HEIGHT
                );

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
            this->hexField.update(mouse);
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

            target.draw(this->hexLabel, states);
            target.draw(this->hexField, states);
        }

        CursorType getCursor() override
        {
            if (!this->enabled)
                return CursorType::ARROW;
            if (this->open && (this->red.dragging || this->green.dragging || this->blue.dragging))
                return CursorType::HAND;
            if (this->open && this->hexField.getCursor() != CursorType::ARROW)
                return this->hexField.getCursor();
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

            this->syncFromColor();
            this->changed = true;
            if (this->onChange)
                this->onChange(this->color);
        }

        // Push the current colour back into every control. The hex field is left
        // alone while it is focused so a resync never clobbers what the user is
        // mid-way through typing.
        void syncFromColor()
        {
            this->red.setValue(this->color.r * 255.f);
            this->green.setValue(this->color.g * 255.f);
            this->blue.setValue(this->color.b * 255.f);
            if (!this->hexField.focused)
                this->hexField.setValue(this->formatHex());
        }

        // Parse the hex field and, when it holds a complete colour, apply it to
        // the sliders / preview and fire onChange - without resyncing the field
        // itself (it is focused, so syncFromColor leaves its text untouched).
        void tryApplyHex()
        {
            ColorF parsed;
            if (!ColorPicker::parseHexColor(this->hexField.value, parsed))
                return;
            this->color = parsed;
            this->syncFromColor();
            this->changed = true;
            if (this->onChange)
                this->onChange(this->color);
        }

        std::string formatHex() const
        {
            const Color c = this->color.toColor();
            char buffer[8];
            std::snprintf(buffer, sizeof(buffer), "#%02X%02X%02X",
                static_cast<unsigned>(c.r),
                static_cast<unsigned>(c.g),
                static_cast<unsigned>(c.b));
            return (buffer);
        }

        static bool isHexDigit(char c)
        {
            return ((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F'));
        }

        // Accepts any prefix of a valid "#RRGGBB": an optional leading '#'
        // followed by at most six hex digits. Used to gate keystrokes.
        static bool isHexInput(const std::string &text)
        {
            size_t i = (!text.empty() && text[0] == '#') ? 1 : 0;
            size_t digits = 0;
            for (; i < text.size(); ++i)
            {
                if (!isHexDigit(text[i]))
                    return (false);
                ++digits;
            }
            return (digits <= 6);
        }

        // A complete colour needs exactly six hex digits (the '#' is optional).
        static bool parseHexColor(const std::string &text, ColorF &out)
        {
            const std::string hex = (!text.empty() && text[0] == '#') ? text.substr(1) : text;
            if (hex.size() != 6)
                return (false);
            for (const char c : hex)
                if (!isHexDigit(c))
                    return (false);
            const unsigned long value = std::stoul(hex, nullptr, 16);
            out.r = static_cast<float>((value >> 16) & 0xFF) / 255.f;
            out.g = static_cast<float>((value >> 8) & 0xFF) / 255.f;
            out.b = static_cast<float>(value & 0xFF) / 255.f;
            return (true);
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