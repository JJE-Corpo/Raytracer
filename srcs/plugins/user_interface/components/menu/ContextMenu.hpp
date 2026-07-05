//
// Created by jazema on 7/5/26.
//
// A floating popup menu opened at the cursor (right-click on a hierarchy row or
// on an object in the viewport). Unlike Menu it has no header/toggle: the owner
// fills it with (label, action) entries and calls openAt(x, y). It renders and
// captures above everything else (zlayer::MENU) while shown, so a click on an
// item runs it and closes, and a click anywhere else just dismisses the menu
// without leaking to the panels/viewport behind it.
//

#ifndef CONTEXTMENU_HPP
#define CONTEXTMENU_HPP

#include <functional>
#include <string>
#include <utility>
#include <vector>

#include "MenuItem.hpp"
#include "../../Component.hpp"
#include "../../Theme.hpp"

namespace rc
{
    class ContextMenu : public Component
    {
        public:
            sf::RectangleShape panel;
            std::vector<MenuItem> items;

            bool open = false;

            void setFont(sf::Font &font) override
            {
                this->_font = &font;
                for (auto &item : this->items)
                    item.setFont(font);
            }

            // Replace the entries and show the menu with its top-left at (x, y).
            void openAt(float x, float y, const std::vector<std::pair<std::string, std::function<void()>>> &entries)
            {
                this->items.clear();
                for (const auto &entry : entries)
                {
                    MenuItem item;
                    if (this->_font)
                        item.setFont(*this->_font);
                    item.setLabel(entry.first);
                    item.onClick = entry.second;
                    this->items.push_back(item);
                }
                this->layout(x, y);
                this->open = true;
            }

            void close()
            {
                this->open = false;
            }

            void layout(float x, float y)
            {
                const float itemHeight = 24.f;
                const float width = 160.f;

                this->panel.setPosition(x, y);
                this->panel.setSize({width, itemHeight * static_cast<float>(this->items.size())});
                this->panel.setFillColor(theme::BG_ITEM);
                this->panel.setOutlineThickness(1.f);
                this->panel.setOutlineColor(theme::OUTLINE_MID);

                float offsetY = y;
                for (auto &item : this->items)
                {
                    item.layout(x, offsetY, width, itemHeight);
                    offsetY += itemHeight;
                }
            }

            void update(sf::Vector2i mouse) override
            {
                if (!this->open)
                    return;
                for (auto &item : this->items)
                    item.update(mouse);
            }

            void draw(sf::RenderTarget &target, sf::RenderStates states) const override
            {
                if (!this->open)
                    return;
                target.draw(this->panel, states);
                for (auto &item : this->items)
                    target.draw(item, states);
            }

            sf::FloatRect getBounds() const override
            {
                if (!this->open)
                    return (sf::FloatRect());
                return (this->panel.getGlobalBounds());
            }

            bool contains(sf::Vector2i point) const override
            {
                if (!this->open)
                    return (false);
                return (this->panel.getGlobalBounds().contains(static_cast<sf::Vector2f>(point)));
            }

            int zLayer() const override
            {
                return (zlayer::MENU);
            }

            bool isCapturing() const override
            {
                return (this->open);
            }

            bool handleEvent(const sf::Event &event, const sf::Vector2i mouse) override
            {
                if (!this->open)
                    return (false);

                // A press on an item runs it and closes; a press anywhere else
                // dismisses the menu. Either way the press is consumed so it never
                // reaches the panels or the viewport behind the popup.
                if (event.type == sf::Event::MouseButtonPressed)
                {
                    for (auto &item : this->items)
                    {
                        if (item.handleEvent(event, mouse))
                        {
                            this->open = false;
                            return (true);
                        }
                    }
                    this->open = false;
                    return (true);
                }
                return (false);
            }

        private:
            sf::Font *_font = nullptr;
    };
}

#endif
