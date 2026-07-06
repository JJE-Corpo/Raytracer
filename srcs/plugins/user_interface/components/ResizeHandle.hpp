//
// Created by jazema on 7/1/26.
//

#ifndef RESIZEHANDLE_HPP
#define RESIZEHANDLE_HPP

#include <algorithm>
#include <functional>
#include <SFML/Graphics.hpp>

#include "../Component.hpp"
#include "../Theme.hpp"

namespace rc
{
    struct ResizeHandle : Component
    {
        std::function<void(float)> onResize;

        bool dragging = false;

        static constexpr float GRAB = 5.f;    // half-width of the interactive strip
        static constexpr float VISUAL = 1.f;  // thickness of the drawn divider line

        void setBounds(float x, float top, float height)
        {
            this->_x = x;
            this->_bounds = sf::FloatRect(x - GRAB, top, GRAB * 2.f, height);
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
                this->apply(mouse.x);
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
                this->apply(mouse.x);
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
                this->apply(mouse.x);
                return (true);
            }
            return (false);
        }

        void draw(sf::RenderTarget &target, sf::RenderStates states) const override
        {
            sf::RectangleShape line;
            line.setPosition(this->_x - VISUAL * 0.5f, this->_bounds.top);
            line.setSize({VISUAL, this->_bounds.height});
            line.setFillColor(this->dragging || this->hovered ? theme::ACCENT : theme::BG_CONTROL);
            target.draw(line, states);
        }

        CursorType getCursor() override
        {
            if (this->dragging || this->hovered)
                return (CursorType::RESIZE_H);
            return (CursorType::ARROW);
        }

    private:
        float _x = 0.f;
        float _min = 0.f;
        float _max = 100000.f;
        sf::FloatRect _bounds;

        void apply(int mouseX)
        {
            const float value = std::clamp(static_cast<float>(mouseX), this->_min, this->_max);
            if (this->onResize)
                this->onResize(value);
        }
    };
}

#endif
