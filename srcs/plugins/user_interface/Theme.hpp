//
// Created by jazema on 5/9/26.
//

#ifndef THEME_HPP
#define THEME_HPP

#include <SFML/Graphics.hpp>

namespace rc
{
    namespace theme
    {
        inline sf::Color withAlpha(const sf::Color &color, sf::Uint8 alpha)
        {
            return sf::Color(color.r, color.g, color.b, alpha);
        }

        const sf::Color BG_WINDOW = sf::Color(30, 30, 30);
        const sf::Color BG_PANEL = sf::Color(28, 28, 28);
        const sf::Color BG_BAR = sf::Color(35, 35, 35);
        const sf::Color BG_ITEM = sf::Color(40, 40, 40);
        const sf::Color BG_ITEM_ALT = sf::Color(45, 45, 45);
        const sf::Color BG_HOVER = sf::Color(50, 50, 50);
        const sf::Color BG_CONTROL = sf::Color(60, 60, 60);
        const sf::Color BG_CONTROL_HOVER = sf::Color(70, 70, 70);
        const sf::Color BG_DISABLED = sf::Color(50, 50, 50);
        const sf::Color BG_POPUP = sf::Color(34, 34, 34);
        const sf::Color SELECTION_BG = sf::Color(55, 70, 90);

        const sf::Color TEXT_MAIN = sf::Color(220, 220, 220);
        const sf::Color TEXT_DIM = sf::Color(150, 150, 150);
        const sf::Color TEXT_SUBTLE = sf::Color(180, 180, 180);
        const sf::Color TEXT_HINT = sf::Color(200, 200, 200);
        const sf::Color TEXT_WHITE = sf::Color(255, 255, 255);

        const sf::Color ACCENT = sf::Color(0, 170, 255);
        const sf::Color CHECKED = sf::Color(0, 180, 120);

        const sf::Color TRACK = sf::Color(55, 55, 55);
        const sf::Color THUMB = sf::Color(230, 230, 230);
        const sf::Color OUTLINE = sf::Color(30, 30, 30);
        const sf::Color OUTLINE_MID = sf::Color(80, 80, 80);
        const sf::Color OUTLINE_SOFT = sf::Color(90, 90, 90);
        const sf::Color OUTLINE_LIGHT = sf::Color(100, 100, 100);
        const sf::Color OUTLINE_HOVER = sf::Color(180, 180, 180);

        const sf::Color TOAST_SUCCESS = sf::Color(40, 120, 80);
        const sf::Color TOAST_ERROR = sf::Color(120, 40, 40);
        const sf::Color TOAST_WARNING = sf::Color(120, 100, 40);
        const sf::Color TOAST_DEFAULT = sf::Color(40, 40, 40);

        const sf::Color BUTTON_PRESSED = sf::Color(20, 120, 180);
    }
}

#endif