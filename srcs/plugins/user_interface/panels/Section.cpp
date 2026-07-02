//
// Created by jazema on 7/1/26.
//

#include "Section.hpp"

#include <algorithm>

#include "../Theme.hpp"

namespace rc
{
    void Section::setFont(sf::Font &font)
    {
        this->_title.setFont(font);
        this->_title.setCharacterSize(13);
        this->_title.setFillColor(theme::TEXT_MAIN);
    }

    void Section::setTitle(const std::string &title)
    {
        this->_title.setString(title);
    }

    void Section::place(float x, float y, float width, float bodyHeight)
    {
        this->_headerRect = sf::FloatRect(x, y, width, HEADER_H);
        this->_fullRect = sf::FloatRect(x, y, width, HEADER_H + bodyHeight);

        this->_title.setPosition(x + 22.f, y + (HEADER_H - 15.f) * 0.5f);

        this->body.setRect(sf::FloatRect(x + this->padX, y + HEADER_H + PAD_Y,
            std::max(0.f, width - 2.f * this->padX), std::max(0.f, bodyHeight - PAD_Y)));
        this->body.layout();
    }

    float Section::bodyBottom() const
    {
        return (this->_fullRect.top + this->_fullRect.height);
    }

    float Section::bodyTop() const
    {
        return (this->_fullRect.top + HEADER_H);
    }

    float Section::bodyHeight() const
    {
        return (this->_fullRect.height - HEADER_H);
    }

    sf::FloatRect Section::getBounds() const
    {
        return (this->_fullRect);
    }

    bool Section::isCapturing() const
    {
        return (this->body.isCapturing());
    }

    void Section::update(sf::Vector2i mouse)
    {
        this->hovered = this->_fullRect.contains(static_cast<float>(mouse.x), static_cast<float>(mouse.y));
        this->body.update(mouse);
    }

    bool Section::handleEvent(const sf::Event &event, const sf::Vector2i mouse)
    {
        return (this->body.handleEvent(event, mouse));
    }

    void Section::draw(sf::RenderTarget &target, sf::RenderStates states) const
    {
        sf::RectangleShape headerBg;
        headerBg.setPosition(this->_headerRect.left, this->_headerRect.top);
        headerBg.setSize({this->_headerRect.width, this->_headerRect.height});
        headerBg.setFillColor(theme::BG_BAR);
        target.draw(headerBg, states);

        target.draw(this->_title, states);

        this->body.draw(target, states);
    }

    void Section::drawOverlay(sf::RenderTarget &target, sf::RenderStates states) const
    {
        this->body.drawOverlay(target, states);
    }

    CursorType Section::getCursor()
    {
        return (this->body.getCursor());
    }
}
