//
// Created by jazema on 7/2/26.
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
            sf::Cursor _move;
    };
}

#endif
