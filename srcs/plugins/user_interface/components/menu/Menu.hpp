//
// Created by jazema on 5/6/26.
//

#ifndef MENU_HPP
#define MENU_HPP
#include "MenuItem.hpp"
#include "../../Component.hpp"
#include "../../Theme.hpp"

namespace rc
{
    class Menu : public Component
    {
        public:
            sf::RectangleShape header;
            sf::RectangleShape panel;
            sf::Text           text;

            std::vector<MenuItem> items;

            bool open = false;

            void setFont(sf::Font &font) override
            {
                text.setFont(font);
                for (auto &i: items)
                    i.setFont(font);
            }

            void setLabel(const std::string &str)
            {
                text.setString(str);
                text.setCharacterSize(14);
                text.setFillColor(theme::TEXT_WHITE);
            }

            float getWidth() const
            {
                return text.getLocalBounds().width + 20.f;
            }

            void layout(float x, float y)
            {
                float w = getWidth();
                float h = 24.f;

                header.setPosition(x, y);
                header.setSize({w, h});
                header.setFillColor(theme::BG_BAR);

                text.setPosition(x + 10, y + 4);

                float panelWidth = 160.f;
                float itemHeight = 24.f;

                panel.setPosition(x, y + h);
                panel.setSize({panelWidth, itemHeight * items.size()});
                panel.setFillColor(theme::BG_ITEM);

                float offsetY = y + h;
                for (auto &item: items)
                {
                    item.layout(x, offsetY, panelWidth, itemHeight);
                    offsetY += itemHeight;
                }
            }

            void update(sf::Vector2i mouse) override
            {
                hovered = header.getGlobalBounds().contains((sf::Vector2f) mouse);

                if (open)
                {
                    for (auto &i: items)
                        i.update(mouse);
                }

                header.setFillColor(hovered || open ? theme::BG_CONTROL : theme::BG_BAR);
            }

            void draw(sf::RenderWindow &window) override
            {
                window.draw(header);
                window.draw(text);

                if (open)
                {
                    window.draw(panel);
                    for (auto &i: items)
                        i.draw(window);
                }
            }

            void handleEvent(const sf::Event &event, const sf::Vector2i mouse) override
            {
                if (!open)
                    return;

                for (auto &i: items)
                {
                    i.handleEvent(event, mouse);
                    if (i.justClicked)
                    {
                        open = false;
                        return;
                    }
                }
            }
    };
}

#endif
