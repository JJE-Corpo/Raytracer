//
// Created by jazema on 7/2/26.
//
// Owns every system cursor the UI can show and applies the right one to a
// window for a given CursorType. Loaded once by UserInterface and shared by
// every screen (via AScreen::setCursorManager), so cursor handling isn't tied
// to one specific screen and isn't reloaded per-screen.
//

#ifndef CURSORMANAGER_HPP
#define CURSORMANAGER_HPP

#include <SFML/Graphics.hpp>

#include "CursorType.hpp"

namespace rc
{
    class CursorManager
    {
        public:
            void load();
            void apply(sf::RenderWindow &window, CursorType type) const;

        private:
            sf::Cursor _arrow;
            sf::Cursor _cross;
            sf::Cursor _hand;
            sf::Cursor _text;
            sf::Cursor _notAllowed;
            sf::Cursor _resizeH;
            sf::Cursor _resizeV;
    };
}

#endif
