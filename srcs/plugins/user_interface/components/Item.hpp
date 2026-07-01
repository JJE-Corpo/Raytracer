#ifndef ITEM_HPP
#define ITEM_HPP

#include <string>
#include <SFML/Graphics.hpp>

#include "../Component.hpp"
#include "../Theme.hpp"

namespace rc
{
    struct Item : Component
    {
        std::string label;
        int depth = 0;
        const void *payload = nullptr;
        bool selectable = false;
        bool selected = false;
        bool hovered = false;
        bool hidden = false;
        sf::FloatRect buttonBounds;
        sf::FloatRect bounds;

        sf::Font *font;

        Item() = default;

        sf::FloatRect getBounds() const override
        {
            return bounds;
        }

        void setFont(sf::Font &font) override
        {
            this->font = &font;
        }

        void setLabel(const std::string &text)
        {
            this->label = text;
        }

        void layout(float x, float y, float width, float height)
        {
            this->bounds = sf::FloatRect(x, y, width, height);
            this->buttonBounds = sf::FloatRect(x, y, width, height);
        }

        void setPayload(const void *data)
        {
            this->payload = data;
        }

        void setSelectable(bool v)
        {
            this->selectable = v;
        }

        void setSelected(bool v)
        {
            this->selected = v;
        }

        void setHidden(bool v)
        {
            this->hidden = v;
        }

        void update(sf::Vector2i mouse) override
        {
            this->hovered = getBounds().contains((float)mouse.x, (float)mouse.y);
        }

        bool handleEvent(const sf::Event &event, const sf::Vector2i mouse) override
        {
            (void)mouse;
            if (event.type == sf::Event::MouseButtonPressed
                && event.mouseButton.button == sf::Mouse::Left
                && this->buttonBounds.contains((float)mouse.x, (float)mouse.y))
            {
                hidden = !hidden;
                return (true);
            }
            return (false);
        }

        void draw(sf::RenderTarget &target, sf::RenderStates states) const override
        {
            sf::RectangleShape bg;
            bg.setPosition({bounds.left, bounds.top});
            bg.setSize({bounds.width, bounds.height});

            if (selected)
                bg.setFillColor(theme::SELECTION_BG);
            else if (hovered)
                bg.setFillColor(theme::BG_HOVER);
            else
                bg.setFillColor(theme::BG_ITEM);

            target.draw(bg, states);

            if (selected)
            {
                sf::RectangleShape accent;
                accent.setPosition({bounds.left, bounds.top});
                accent.setSize({3.f, bounds.height});
                accent.setFillColor(theme::ACCENT);
                target.draw(accent, states);
            }

            sf::Text text;
            text.setFont(*font);
            text.setCharacterSize(12);
            text.setFillColor(hidden ? theme::TEXT_DIM : theme::TEXT_MAIN);
            text.setString(label);
            text.setPosition({bounds.left + 8.f, bounds.top + 2.f});
            target.draw(text, states);
        }
    };
}

#endif