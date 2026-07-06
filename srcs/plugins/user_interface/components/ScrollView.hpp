//
// Created by jazema on 7/1/26.
//

#ifndef SCROLLVIEW_HPP
#define SCROLLVIEW_HPP

#include <algorithm>
#include <functional>
#include <SFML/Graphics.hpp>

#include "../Component.hpp"
#include "../Theme.hpp"

namespace rc
{
    struct ScrollView : Component
    {
        Component *content = nullptr;
        std::function<void(float x, float y, float width)> layoutContent;
        std::function<float()> contentHeight;

        static constexpr float SCROLLBAR_W = 8.f;
        static constexpr float SCROLLBAR_GAP = 4.f;
        static constexpr float MIN_THUMB = 20.f;
        static constexpr float WHEEL_STEP = 32.f;

        void setRect(const sf::FloatRect &rect)
        {
            this->_rect = rect;
        }

        float scrollOffset() const
        {
            return (this->_scrollY);
        }

        void layout()
        {
            const float width = std::max(0.f, this->_rect.width - SCROLLBAR_W - SCROLLBAR_GAP);
            if (this->layoutContent)
                this->layoutContent(this->_rect.left, this->_rect.top, width);
            this->_contentHeight = this->contentHeight ? this->contentHeight() : 0.f;
            this->_scrollY = std::clamp(this->_scrollY, 0.f, this->maxScroll());
        }

        sf::FloatRect getBounds() const override
        {
            return (this->_rect);
        }

        bool isCapturing() const override
        {
            return (this->_scrollbarDragging || (this->content && this->content->isCapturing()));
        }

        void update(sf::Vector2i mouse) override
        {
            if (this->_scrollbarDragging)
                this->dragThumbTo(static_cast<float>(mouse.y));

            this->hovered = this->_rect.contains(static_cast<float>(mouse.x), static_cast<float>(mouse.y));

            if (this->content)
                this->content->update(this->toContent(mouse));
        }

        bool handleEvent(const sf::Event &event, const sf::Vector2i mouse) override
        {
            const bool inRect = this->_rect.contains(static_cast<float>(mouse.x), static_cast<float>(mouse.y));

            if (event.type == sf::Event::MouseWheelScrolled && inRect && this->scrollable())
            {
                this->_scrollY = std::clamp(this->_scrollY - event.mouseWheelScroll.delta * WHEEL_STEP, 0.f, this->maxScroll());
                return (true);
            }

            if (event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Left
                && this->scrollable() && this->thumbRect().contains(static_cast<float>(mouse.x), static_cast<float>(mouse.y)))
            {
                this->_scrollbarDragging = true;
                this->_scrollbarDragOffset = static_cast<float>(mouse.y) - this->thumbRect().top;
                return (true);
            }
            if (event.type == sf::Event::MouseButtonReleased && event.mouseButton.button == sf::Mouse::Left && this->_scrollbarDragging)
            {
                this->_scrollbarDragging = false;
                return (true);
            }

            if (this->content && (inRect || this->content->isCapturing()))
                return (this->content->handleEvent(event, this->toContent(mouse)));
            return (false);
        }

        void draw(sf::RenderTarget &target, sf::RenderStates states) const override
        {
            const sf::Vector2u size = target.getSize();
            if (size.x == 0 || size.y == 0 || !this->content)
                return;

            const float W = static_cast<float>(size.x);
            const float H = static_cast<float>(size.y);
            const float visW = std::min(this->_rect.width, W - this->_rect.left);
            const float visH = std::min(this->_rect.height, H - this->_rect.top);
            if (visW <= 0.f || visH <= 0.f)
                return;

            const sf::View saved = target.getView();

            sf::View clip;
            clip.setSize(visW, visH);
            clip.setCenter(this->_rect.left + visW * 0.5f, this->_rect.top + this->_scrollY + visH * 0.5f);
            clip.setViewport(sf::FloatRect(this->_rect.left / W, this->_rect.top / H, visW / W, visH / H));

            target.setView(clip);
            target.draw(*this->content, states);
            target.setView(saved);

            this->drawScrollbar(target);
        }

        void drawOverlay(sf::RenderTarget &target, sf::RenderStates states) const override
        {
            if (!this->content)
                return;
            states.transform.translate(0.f, -this->_scrollY);
            this->content->drawOverlay(target, states);
        }

        CursorType getCursor() override
        {
            if (this->content)
                return (this->content->getCursor());
            return (CursorType::ARROW);
        }

    private:
        sf::FloatRect _rect;
        float _scrollY = 0.f;
        float _contentHeight = 0.f;
        bool _scrollbarDragging = false;
        float _scrollbarDragOffset = 0.f;

        float maxScroll() const
        {
            return (std::max(0.f, this->_contentHeight - this->_rect.height));
        }

        bool scrollable() const
        {
            return (this->_contentHeight > this->_rect.height + 0.5f);
        }

        sf::Vector2i toContent(sf::Vector2i mouse) const
        {
            if (!this->_rect.contains(static_cast<float>(mouse.x), static_cast<float>(mouse.y))
                && !(this->content && this->content->isCapturing()))
                return (sf::Vector2i(-100000, -100000));
            return (sf::Vector2i(mouse.x, mouse.y + static_cast<int>(this->_scrollY)));
        }

        sf::FloatRect trackRect() const
        {
            return (sf::FloatRect(this->_rect.left + this->_rect.width - SCROLLBAR_W, this->_rect.top, SCROLLBAR_W, this->_rect.height));
        }

        sf::FloatRect thumbRect() const
        {
            const sf::FloatRect track = this->trackRect();
            const float ms = this->maxScroll();
            float thumbH = track.height * (this->_rect.height / std::max(1.f, this->_contentHeight));
            thumbH = std::clamp(thumbH, MIN_THUMB, track.height);
            const float t = ms > 0.f ? this->_scrollY / ms : 0.f;
            const float thumbY = track.top + (track.height - thumbH) * t;
            return (sf::FloatRect(track.left, thumbY, track.width, thumbH));
        }

        void dragThumbTo(float mouseY)
        {
            const sf::FloatRect track = this->trackRect();
            const sf::FloatRect thumb = this->thumbRect();
            const float span = track.height - thumb.height;
            if (span <= 0.f)
                return;
            float top = std::clamp(mouseY - this->_scrollbarDragOffset, track.top, track.top + span);
            const float t = (top - track.top) / span;
            this->_scrollY = std::clamp(t * this->maxScroll(), 0.f, this->maxScroll());
        }

        void drawScrollbar(sf::RenderTarget &target) const
        {
            if (!this->scrollable())
                return;

            const sf::FloatRect track = this->trackRect();
            sf::RectangleShape trackShape;
            trackShape.setPosition(track.left, track.top);
            trackShape.setSize({track.width, track.height});
            trackShape.setFillColor(theme::BG_ITEM);
            target.draw(trackShape);

            const sf::FloatRect thumb = this->thumbRect();
            sf::RectangleShape thumbShape;
            thumbShape.setPosition(thumb.left, thumb.top);
            thumbShape.setSize({thumb.width, thumb.height});
            thumbShape.setFillColor(this->_scrollbarDragging ? theme::ACCENT : theme::BG_CONTROL);
            target.draw(thumbShape);
        }
    };
}

#endif
