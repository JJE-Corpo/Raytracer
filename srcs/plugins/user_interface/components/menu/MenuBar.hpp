//
// Created by jazema on 5/6/26.
//

#ifndef MENUBAR_HPP
#define MENUBAR_HPP
#include "Menu.hpp"
#include "../../Component.hpp"
#include "../../Theme.hpp"

namespace rc
{
    class Menu;

    class MenuBar : public Component
    {
        public:
            sf::RectangleShape bar;
            std::vector<Menu> menus;

            Menu *activeMenu = nullptr;

            void setFont(sf::Font &font) override
            {
                for (auto &m : menus)
                    m.setFont(font);
            }

            void layout(float width)
            {
                this->bar.setPosition(0, 0);
                this->bar.setSize({width, 28});
                this->bar.setFillColor(theme::BG_BAR);

                float x = 5.f;

                for (auto &menu : this->menus)
                {
                    menu.layout(x, 2.f);
                    x += menu.getWidth() + 5.f;
                }
            }

            void update(sf::Vector2i mouse) override
            {
                bool anyHovered = false;

                for (auto &menu : this->menus)
                {
                    menu.update(mouse);

                    if (menu.hovered)
                    {
                        anyHovered = true;

                        if (activeMenu != &menu)
                        {
                            if (activeMenu)
                                activeMenu->open = false;

                            menu.open = true;
                            activeMenu = &menu;
                        }
                    }
                }

                if (!anyHovered && activeMenu)
                {
                    bool insidePanel = activeMenu->panel.getGlobalBounds().contains((sf::Vector2f)mouse);

                    if (!insidePanel)
                    {
                        activeMenu->open = false;
                        activeMenu = nullptr;
                    }
                }

                if (activeMenu)
                    activeMenu->update(mouse);
            }

            void draw(sf::RenderWindow &window) override
            {
                window.draw(bar);

                for (auto &menu : menus)
                    menu.draw(window);
            }

            void handleEvent(const sf::Event &event, const sf::Vector2i mouse) override
            {
                if (activeMenu)
                    activeMenu->handleEvent(event, mouse);
            }

            bool isOpen() const
            {
                for (const auto &menu : menus)
                    if (menu.open)
                        return true;
                return false;
            }
    };
}

#endif
