//
// Created by jazema on 5/5/26.
//

#ifndef COMPONENT_HPP
#define COMPONENT_HPP

#include <SFML/Graphics.hpp>

#include "CursorType.hpp"

namespace rc
{
    namespace zlayer
    {
        constexpr int BASE    = 0;   // panels, static controls
        constexpr int POPUP   = 10;  // open dropdowns / color pickers
        constexpr int MENU    = 20;  // the menu bar and its open menus
        constexpr int OVERLAY = 30;  // modal-ish overlays
    }

    struct Component : public sf::Drawable
    {
        bool enabled = true;
        bool hovered = false;
        bool visible = true;

        virtual ~Component() = default;

        void draw(sf::RenderTarget &target, sf::RenderStates states) const override = 0;

        virtual void drawOverlay(sf::RenderTarget &target, sf::RenderStates states) const
        {
            (void)target;
            (void)states;
        }

        virtual void update(sf::Vector2i mouse)
        {
            (void)mouse;
        }

        virtual bool handleEvent(const sf::Event &event, const sf::Vector2i mouse)
        {
            (void)event;
            (void)mouse;
            return (false);
        }

        virtual void setFont(sf::Font &font)
        {
            (void)font;
        }

        virtual CursorType getCursor()
        {
            return (CursorType::ARROW);
        }

        // --- overlap arbitration -------------------------------------------

        virtual sf::FloatRect getBounds() const
        {
            return (sf::FloatRect());
        }

        virtual bool contains(sf::Vector2i point) const
        {
            const sf::FloatRect bounds = this->getBounds();
            if (bounds.width <= 0.f || bounds.height <= 0.f)
                return (false);
            return (bounds.contains(static_cast<float>(point.x), static_cast<float>(point.y)));
        }

        virtual int zLayer() const
        {
            return (zlayer::BASE);
        }

        virtual bool isCapturing() const
        {
            return (false);
        }
    };
}

#endif
