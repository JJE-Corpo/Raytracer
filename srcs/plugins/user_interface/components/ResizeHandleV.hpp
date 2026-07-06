//
// Created by jazema on 7/1/26.
//

#ifndef RESIZEHANDLEV_HPP
#define RESIZEHANDLEV_HPP

#include <algorithm>
#include <functional>
#include <SFML/Graphics.hpp>

#include "../Component.hpp"
#include "../Theme.hpp"

namespace rc
{
    struct ResizeHandleV : Component
    {
        std::function<void(float)> onResize;

        bool dragging = false;

        static constexpr float GRAB = 4.f;    // half-height of the interactive strip
        static constexpr float VISUAL = 1.f;  // thickness of the drawn divider line

        void setBounds(float y, float left, float width)
        {
            this->_y = y;
            this->_bounds = sf::FloatRect(left, y - GRAB, width, GRAB * 2.f);
        }

        void setRange(float lo, float hi)
        {
            this->_min = lo;
            this->_max = hi;
        }

        sf::FloatRect getBounds() const override
        {
            return (this->_bounds);
        }

        bool isCapturing() const override
        {
            return (this->dragging);
        }

        int zLayer() const override
        {
            return (zlayer::POPUP);
        }

        void update(sf::Vector2i mouse) override
        {
            this->hovered = this->_bounds.contains(static_cast<float>(mouse.x), static_cast<float>(mouse.y));
            if (this->dragging)
                this->apply(mouse.y);
        }

        bool handleEvent(const sf::Event &event, const sf::Vector2i mouse) override
        {
            if (!this->enabled)
                return (false);

            if (event.type == sf::Event::MouseButtonPressed
                && event.mouseButton.button == sf::Mouse::Left
                && this->_bounds.contains(static_cast<float>(mouse.x), static_cast<float>(mouse.y)))
            {
                this->dragging = true;
                this->apply(mouse.y);
                return (true);
            }
            if (event.type == sf::Event::MouseButtonReleased
                && event.mouseButton.button == sf::Mouse::Left)
            {
                const bool wasDragging = this->dragging;
                this->dragging = false;
                return (wasDragging);
            }
            if (event.type == sf::Event::MouseMoved && this->dragging)
            {
                this->apply(mouse.y);
                return (true);
            }
            return (false);
        }

        void draw(sf::RenderTarget &target, sf::RenderStates states) const override
        {
            sf::RectangleShape line;
            line.setPosition(this->_bounds.left, this->_y - VISUAL * 0.5f);
            line.setSize({this->_bounds.width, VISUAL});
            line.setFillColor(this->dragging || this->hovered ? theme::ACCENT : theme::BG_CONTROL);
            target.draw(line, states);
        }

        CursorType getCursor() override
        {
            if (this->dragging || this->hovered)
                return (CursorType::RESIZE_V);
            return (CursorType::ARROW);
        }

    private:
        float _y = 0.f;
        float _min = 0.f;
        float _max = 100000.f;
        sf::FloatRect _bounds;

        void apply(int mouseY)
        {
            const float value = std::clamp(static_cast<float>(mouseY), this->_min, this->_max);
            if (this->onResize)
                this->onResize(value);
        }
    };
}

#endif
