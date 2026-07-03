//
// Created by jazema on 5/5/26.
//

#ifndef COMPONENT_HPP
#define COMPONENT_HPP

#include <SFML/Graphics.hpp>

#include "CursorType.hpp"

namespace rc
{
    // Z-layers used to arbitrate overlapping components. Higher layers are drawn
    // on top and are offered events first. Popups raise their layer while open.
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

        // Second, unclipped drawing pass for content that must escape a parent's
        // scroll/clip region (open dropdown and color-picker pop-ups). Default is
        // a no-op; the sidebar runs this pass after drawing every clipped section.
        virtual void drawOverlay(sf::RenderTarget &target, sf::RenderStates states) const
        {
            (void)target;
            (void)states;
        }

        virtual void update(sf::Vector2i mouse)
        {
            (void)mouse;
        }

        // Handle an event. Returns true when the event was consumed, so the
        // event router stops offering it to components underneath.
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

        // Interactive bounds of the component in window space. An empty rect
        // (width/height == 0) means "unspecified": the router will offer
        // pointer events to the component regardless of the cursor position,
        // letting the component do its own hit-testing (backwards compatible).
        virtual sf::FloatRect getBounds() const
        {
            return (sf::FloatRect());
        }

        // Hit-test a point against the interactive surface. Components with
        // popups (dropdown, color picker, menu) override this to include the
        // popup area while it is open.
        virtual bool contains(sf::Vector2i point) const
        {
            const sf::FloatRect bounds = this->getBounds();
            if (bounds.width <= 0.f || bounds.height <= 0.f)
                return (false);
            return (bounds.contains(static_cast<float>(point.x), static_cast<float>(point.y)));
        }

        // Higher layers are offered events before (and drawn above) lower ones.
        virtual int zLayer() const
        {
            return (zlayer::BASE);
        }

        // True while the component must keep receiving events regardless of the
        // cursor position: an in-progress drag, an open popup, keyboard focus.
        // Capturing components are always offered events first.
        virtual bool isCapturing() const
        {
            return (false);
        }
    };
}

#endif
