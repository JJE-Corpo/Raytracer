//
// Created by jazema on 7/1/26.
//
// Central event router: given a set of overlapping components, decides which
// single one should receive an event ("select the better component to call").
//
// Rules, applied in order:
//   1. Capturing components (drag in progress, open popup, keyboard focus) are
//      offered the event first, top-most (highest zLayer) first.
//   2. For pointer events, the top-most component whose contains() matches the
//      cursor is offered the event. A component that does not declare bounds is
//      always offered pointer events and does its own hit-testing.
//   3. For non-pointer events (keyboard / text), every component is offered the
//      event in z-order until one consumes it.
// Propagation stops as soon as a component's handleEvent() returns true.
//

#ifndef EVENTROUTER_HPP
#define EVENTROUTER_HPP

#include <algorithm>
#include <vector>

#include <SFML/Graphics.hpp>

#include "Component.hpp"

namespace rc
{
    class EventRouter
    {
        public:
            static bool isPointerEvent(const sf::Event &event)
            {
                switch (event.type)
                {
                    case sf::Event::MouseButtonPressed:
                    case sf::Event::MouseButtonReleased:
                    case sf::Event::MouseMoved:
                    case sf::Event::MouseWheelScrolled:
                    case sf::Event::MouseWheelMoved:
                        return (true);
                    default:
                        return (false);
                }
            }

            // Route one event among the given components (any order). Returns the
            // component that consumed it, or nullptr if none did.
            static Component *route(const std::vector<Component *> &components, const sf::Event &event, const sf::Vector2i mouse)
            {
                std::vector<Component *> order;
                order.reserve(components.size());
                for (Component *component : components)
                {
                    if (component && component->visible && component->enabled)
                        order.push_back(component);
                }

                // Capturers first, then top-most (highest zLayer). stable_sort
                // keeps the caller-provided order as the final tie-break.
                std::stable_sort(order.begin(), order.end(), [](Component *a, Component *b)
                {
                    if (a->isCapturing() != b->isCapturing())
                        return (a->isCapturing());
                    return (a->zLayer() > b->zLayer());
                });

                const bool pointer = isPointerEvent(event);

                for (Component *component : order)
                {
                    // A non-capturing component only gets a pointer event when the
                    // cursor is over its declared bounds. Bounds-less components are
                    // always tried (they hit-test themselves).
                    if (pointer && !component->isCapturing())
                    {
                        const sf::FloatRect bounds = component->getBounds();
                        if (bounds.width > 0.f && bounds.height > 0.f && !component->contains(mouse))
                            continue;
                    }
                    if (component->handleEvent(event, mouse))
                        return (component);
                }
                return (nullptr);
            }
    };
}

#endif
