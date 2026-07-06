//
// Created by jazema on 7/2/26.
//

#include "CursorManager.hpp"

namespace rc
{
    void CursorManager::load()
    {
        this->_arrow.loadFromSystem(sf::Cursor::Arrow);
        this->_cross.loadFromSystem(sf::Cursor::Cross);
        this->_hand.loadFromSystem(sf::Cursor::Hand);
        this->_text.loadFromSystem(sf::Cursor::Text);
        this->_notAllowed.loadFromSystem(sf::Cursor::NotAllowed);
        this->_resizeH.loadFromSystem(sf::Cursor::SizeHorizontal);
        this->_resizeV.loadFromSystem(sf::Cursor::SizeVertical);
        this->_move.loadFromSystem(sf::Cursor::SizeAll);
    }

    void CursorManager::apply(sf::RenderWindow &window, CursorType type) const
    {
        const sf::Cursor &cursor = (type == CursorType::CROSS ? this->_cross :
                        type == CursorType::ARROW ? this->_arrow :
                        type == CursorType::HAND ? this->_hand :
                        type == CursorType::TEXT ? this->_text :
                        type == CursorType::RESIZE_H ? this->_resizeH :
                        type == CursorType::RESIZE_V ? this->_resizeV :
                        type == CursorType::MOVE ? this->_move :
                        this->_notAllowed);

        window.setMouseCursor(cursor);
    }
}
