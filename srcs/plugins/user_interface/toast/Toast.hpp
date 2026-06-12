//
// Created by jazema on 5/2/26.
//

#ifndef TOAST_HPP
#define TOAST_HPP

#include <string>
#include <SFML/Graphics.hpp>

namespace rc
{
    enum class ToastType
    {
        INFO,
        SUCCESS,
        WARNING,
        ERROR,
    };
    struct Toast
    {
        sf::RectangleShape box;
        sf::Text title;
        sf::Text content;

        sf::Clock lifetime;
        float duration = 3.0f;

        float alpha = 0.f;
    };
}

#endif
