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
        const float body_h = this->collapsed ? 0.f : bodyHeight;

        this->_headerRect = sf::FloatRect(x, y, width, HEADER_H);
        this->_fullRect = sf::FloatRect(x, y, width, HEADER_H + body_h);

        this->_title.setPosition(x + 22.f, y + (HEADER_H - 15.f) * 0.5f);

        if (!this->collapsed)
        {
            this->body.setRect(sf::FloatRect(x + PAD_X, y + HEADER_H + PAD_Y,
                std::max(0.f, width - 2.f * PAD_X), std::max(0.f, bodyHeight - PAD_Y)));
            this->body.layout();
        }
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
        return (this->collapsed ? 0.f : this->_fullRect.height - HEADER_H);
    }

    sf::FloatRect Section::getBounds() const
    {
        return (this->_fullRect);
    }

    bool Section::isCapturing() const
    {
        return (!this->collapsed && this->body.isCapturing());
    }

    void Section::update(sf::Vector2i mouse)
    {
        this->_headerHovered = this->_headerRect.contains(static_cast<float>(mouse.x), static_cast<float>(mouse.y));
        this->hovered = this->_fullRect.contains(static_cast<float>(mouse.x), static_cast<float>(mouse.y));
        if (!this->collapsed)
            this->body.update(mouse);
    }

    bool Section::handleEvent(const sf::Event &event, const sf::Vector2i mouse)
    {
        if (event.type == sf::Event::MouseButtonPressed
            && event.mouseButton.button == sf::Mouse::Left
            && this->_headerRect.contains(static_cast<float>(mouse.x), static_cast<float>(mouse.y)))
        {
            this->collapsed = !this->collapsed;
            return (true);
        }
        if (!this->collapsed)
            return (this->body.handleEvent(event, mouse));
        return (false);
    }

    void Section::draw(sf::RenderTarget &target, sf::RenderStates states) const
    {
        sf::RectangleShape headerBg;
        headerBg.setPosition(this->_headerRect.left, this->_headerRect.top);
        headerBg.setSize({this->_headerRect.width, this->_headerRect.height});
        headerBg.setFillColor(this->_headerHovered ? theme::BG_CONTROL : theme::BG_BAR);
        target.draw(headerBg, states);

        target.draw(this->makeChevron(), states);
        target.draw(this->_title, states);

        if (!this->collapsed)
            this->body.draw(target, states);
    }

    void Section::drawOverlay(sf::RenderTarget &target, sf::RenderStates states) const
    {
        if (!this->collapsed)
            this->body.drawOverlay(target, states);
    }

    CursorType Section::getCursor()
    {
        if (this->_headerHovered)
            return (CursorType::HAND);
        if (!this->collapsed)
            return (this->body.getCursor());
        return (CursorType::ARROW);
    }

    sf::ConvexShape Section::makeChevron() const
    {
        const float cx = this->_headerRect.left + 9.f;
        const float cy = this->_headerRect.top + HEADER_H * 0.5f;
        const float s = 4.f;

        sf::ConvexShape chevron;
        chevron.setPointCount(3);
        chevron.setFillColor(theme::TEXT_DIM);
        if (this->collapsed)
        {
            chevron.setPoint(0, {cx - s, cy - s});
            chevron.setPoint(1, {cx + s, cy});
            chevron.setPoint(2, {cx - s, cy + s});
        }
        else
        {
            chevron.setPoint(0, {cx - s, cy - s});
            chevron.setPoint(1, {cx + s, cy - s});
            chevron.setPoint(2, {cx, cy + s});
        }
        return (chevron);
    }
}
