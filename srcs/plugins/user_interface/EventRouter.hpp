//
// Created by jazema on 7/1/26.
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

            static Component *route(const std::vector<Component *> &components, const sf::Event &event, const sf::Vector2i mouse)
            {
                std::vector<Component *> order;
                order.reserve(components.size());
                for (Component *component : components)
                {
                    if (component && component->visible && component->enabled)
                        order.push_back(component);
                }

                std::stable_sort(order.begin(), order.end(), [](Component *a, Component *b)
                {
                    if (a->isCapturing() != b->isCapturing())
                        return (a->isCapturing());
                    return (a->zLayer() > b->zLayer());
                });

                const bool pointer = isPointerEvent(event);

                for (Component *component : order)
                {
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
