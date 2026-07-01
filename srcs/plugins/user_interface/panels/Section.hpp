//
// Created by jazema on 7/1/26.
//
// A sidebar section: a clickable, collapsible header on top of a scrollable body
// (ScrollView) wrapping one panel. The SidebarStack owns a list of these, sizes
// them, and puts draggable dividers between them.
//

#ifndef SECTION_HPP
#define SECTION_HPP

#include <algorithm>
#include <string>
#include <SFML/Graphics.hpp>

#include "../Component.hpp"
#include "../Theme.hpp"
#include "../components/ScrollView.hpp"

namespace rc
{
    struct Section : Component
    {
        std::string id;
        ScrollView body;
        bool collapsed = false;

        static constexpr float HEADER_H = 26.f;
        static constexpr float PAD_X = 14.f;  // horizontal inset of the body content
        static constexpr float PAD_Y = 8.f;   // gap under the header

        void setFont(sf::Font &font) override
        {
            this->_title.setFont(font);
            this->_title.setCharacterSize(13);
            this->_title.setFillColor(theme::TEXT_MAIN);
        }

        void setTitle(const std::string &title)
        {
            this->_title.setString(title);
        }

        // Position the section: header of fixed height on top, body filling
        // bodyHeight below it (0 while collapsed). Called every frame by the stack.
        void place(float x, float y, float width, float bodyHeight)
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

        float fullHeight() const
        {
            return (this->_fullRect.height);
        }

        float bodyBottom() const
        {
            return (this->_fullRect.top + this->_fullRect.height);
        }

        float bodyTop() const
        {
            return (this->_fullRect.top + HEADER_H);
        }

        float bodyHeight() const
        {
            return (this->collapsed ? 0.f : this->_fullRect.height - HEADER_H);
        }

        sf::FloatRect getBounds() const override
        {
            return (this->_fullRect);
        }

        bool isCapturing() const override
        {
            return (!this->collapsed && this->body.isCapturing());
        }

        void update(sf::Vector2i mouse) override
        {
            this->_headerHovered = this->_headerRect.contains(static_cast<float>(mouse.x), static_cast<float>(mouse.y));
            this->hovered = this->_fullRect.contains(static_cast<float>(mouse.x), static_cast<float>(mouse.y));
            if (!this->collapsed)
                this->body.update(mouse);
        }

        bool handleEvent(const sf::Event &event, const sf::Vector2i mouse) override
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

        void draw(sf::RenderTarget &target, sf::RenderStates states) const override
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

        void drawOverlay(sf::RenderTarget &target, sf::RenderStates states) const override
        {
            if (!this->collapsed)
                this->body.drawOverlay(target, states);
        }

        CursorType getCursor() override
        {
            if (this->_headerHovered)
                return (CursorType::HAND);
            if (!this->collapsed)
                return (this->body.getCursor());
            return (CursorType::ARROW);
        }

    private:
        sf::Text _title;
        sf::FloatRect _fullRect;
        sf::FloatRect _headerRect;
        bool _headerHovered = false;

        // A small triangle pointing down (expanded) or right (collapsed).
        sf::ConvexShape makeChevron() const
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
    };
}

#endif
