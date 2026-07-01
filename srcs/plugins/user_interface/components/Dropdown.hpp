//
// Created by jazema on 5/13/26.
//

#ifndef DROPDOWN_HPP
#define DROPDOWN_HPP

#include <functional>
#include <string>
#include <vector>

#include <SFML/Graphics.hpp>

#include "menu/MenuItem.hpp"
#include "../Component.hpp"
#include "../Theme.hpp"

namespace rc
{
    class Dropdown : public Component
    {
        public:
            sf::RectangleShape header;
            sf::RectangleShape panel;
            sf::Text text;

            std::vector<MenuItem> items;

            bool open = false;

            std::function<void(int)> onSelect;

        private:
            std::vector<std::string> _labels;
            std::string _placeholder = "Select";
            sf::Font *_font = nullptr;
            int _selectedIndex = -1;
            float _x = 0.0f;
            float _y = 0.0f;
            float _w = 160.0f;

        public:
            void setFont(sf::Font &font) override
            {
                _font = &font;
                text.setFont(font);
                for (auto &item : items)
                    item.setFont(font);
            }

            void setPlaceholder(const std::string &placeholder)
            {
                _placeholder = placeholder;
                if (_selectedIndex < 0)
                    text.setString(_placeholder);
                text.setCharacterSize(14);
                text.setFillColor(theme::TEXT_WHITE);
            }

            void setOptions(const std::vector<std::string> &labels)
            {
                _labels = labels;
                items.clear();
                items.reserve(_labels.size());

                for (const auto &label : _labels)
                {
                    MenuItem item;
                    item.setLabel(label);
                    if (_font)
                        item.setFont(*_font);
                    items.push_back(item);
                }

                if (_selectedIndex >= static_cast<int>(_labels.size()))
                    setSelectedIndex(-1);
            }

            void setSelectedIndex(int index)
            {
                _selectedIndex = index;
                if (_selectedIndex >= 0 && _selectedIndex < static_cast<int>(_labels.size()))
                    text.setString(_labels[_selectedIndex]);
                else
                    text.setString(_placeholder);
            }

            int getSelectedIndex() const
            {
                return _selectedIndex;
            }

            void layout(float x, float y, float width)
            {
                _x = x;
                _y = y;
                _w = width;

                const float headerHeight = 24.0f;
                const float itemHeight = 24.0f;

                header.setPosition(x, y);
                header.setSize({_w, headerHeight});
                header.setFillColor(theme::BG_BAR);

                text.setPosition(x + 10.0f, y + 4.0f);

                panel.setPosition(x, y + headerHeight);
                panel.setSize({_w, itemHeight * items.size()});
                panel.setFillColor(theme::BG_ITEM);

                float offsetY = y + headerHeight;
                for (auto &item : items)
                {
                    item.layout(x, offsetY, _w, itemHeight);
                    offsetY += itemHeight;
                }
            }

            void update(sf::Vector2i mouse) override
            {
                hovered = header.getGlobalBounds().contains((sf::Vector2f) mouse);

                if (open)
                {
                    for (auto &item : items)
                        item.update(mouse);
                }

                header.setFillColor(hovered || open ? theme::BG_CONTROL : theme::BG_BAR);
            }

            void draw(sf::RenderTarget &target, sf::RenderStates states) const override
            {
                target.draw(header, states);
                target.draw(text, states);

                if (open)
                {
                    target.draw(panel, states);
                    for (auto &item : items)
                        target.draw(item, states);
                }
            }

            void handleEvent(const sf::Event &event, const sf::Vector2i mouse) override
            {
                if (!enabled)
                    return;

                if (event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Left)
                {
                    const bool inHeader = header.getGlobalBounds().contains((sf::Vector2f) mouse);
                    const bool inPanel = open && panel.getGlobalBounds().contains((sf::Vector2f) mouse);

                    if (inHeader)
                    {
                        open = !open;
                        return;
                    }

                    if (open && !inPanel)
                    {
                        open = false;
                        return;
                    }
                }

                if (!open)
                    return;

                for (size_t i = 0; i < items.size(); ++i)
                {
                    auto &item = items[i];
                    item.handleEvent(event, mouse);
                    if (event.type == sf::Event::MouseButtonPressed &&
                        event.mouseButton.button == sf::Mouse::Left &&
                        item.hovered)
                    {
                        setSelectedIndex(static_cast<int>(i));
                        open = false;
                        if (onSelect)
                            onSelect(static_cast<int>(i));
                        return;
                    }
                }
            }

            CursorType getCursor() override
            {
                if (hovered)
                    return CursorType::HAND;

                if (open)
                {
                    for (const auto &item : items)
                    {
                        if (item.hovered)
                            return CursorType::HAND;
                    }
                }

                return CursorType::ARROW;
            }
    };
}

#endif
