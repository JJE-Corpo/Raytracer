//
// Created by jazema on 5/2/26.
//

#ifndef TOASTMANAGER_HPP
#define TOASTMANAGER_HPP

#include <SFML/Graphics.hpp>

#include "Toast.hpp"
#include "../Theme.hpp"

namespace rc
{
    struct ToastManager
    {
        std::vector<Toast> toasts;

        void setFont(sf::Font &font)
        {
            this->_font = &font;
        }

        void push(const std::string &title, const std::string &content, ToastType type = ToastType::INFO)
        {
            Toast t;

            t.lifetime.restart();
            t.title.setFont(*_font);
            t.title.setCharacterSize(15);
            t.title.setFillColor(theme::TEXT_WHITE);
            t.title.setString(title);
            t.title.setStyle(sf::Text::Bold);

            t.content.setFont(*_font);
            t.content.setCharacterSize(13);
            t.content.setFillColor(theme::TEXT_HINT);
            t.content.setString(content);

            switch (type)
            {
                case ToastType::SUCCESS: t.box.setFillColor(theme::TOAST_SUCCESS); break;
                case ToastType::ERROR:   t.box.setFillColor(theme::TOAST_ERROR); break;
                case ToastType::WARNING: t.box.setFillColor(theme::TOAST_WARNING); break;
                default:                 t.box.setFillColor(theme::TOAST_DEFAULT); break;
            }

            toasts.push_back(t);
        }

        void update()
        {
            for (auto &t : toasts)
            {
                float time = t.lifetime.getElapsedTime().asSeconds();

                // fade in
                if (time < 0.2f)
                    t.alpha = time / 0.2f;
                // fade out
                else if (time > t.duration - 0.5f)
                    t.alpha = (t.duration - time) / 0.5f;
                else
                    t.alpha = 1.f;

                t.alpha = std::clamp(t.alpha, 0.f, 1.f);
            }

            // remove expired
            toasts.erase(
                std::remove_if(toasts.begin(), toasts.end(),
                    [](const Toast &t)
                    {
                        return t.lifetime.getElapsedTime().asSeconds() > t.duration;
                    }),
                toasts.end()
            );
        }

        void draw(sf::RenderWindow &window)
        {
            float margin = 20.f;
            float spacing = 10.f;

            float y = window.getSize().y - margin;

            for (int i = (int)toasts.size() - 1; i >= 0; --i)
            {
                auto &t = toasts[i];

                auto titleBounds = t.title.getLocalBounds();
                auto contentBounds = t.content.getLocalBounds();

                float padding = 10.f;
                float spacingY = 4.f;

                float w = std::max(titleBounds.width, contentBounds.width) + 2 * padding;
                float h = titleBounds.height + contentBounds.height + spacingY + 2 * padding;

                float x = window.getSize().x - w - margin;
                y -= h;

                float time = t.lifetime.getElapsedTime().asSeconds();

                float enter = std::clamp(time / 0.2f, 0.f, 1.f);
                float exit  = std::clamp((t.duration - time) / 0.3f, 0.f, 1.f);

                auto easeOut = [](float x)
                {
                    return 1.f - (1.f - x) * (1.f - x);
                };

                float slide = easeOut(std::min(enter, exit));
                x += (1.f - slide) * 50.f;

                t.box.setSize({w, h});
                t.box.setPosition(x, y);

                float textX = x + padding;
                float textY = y + padding;

                t.title.setPosition(textX, textY);

                t.content.setPosition(
                    textX,
                    textY + titleBounds.height + spacingY
                );

                // apply fade
                sf::Uint8 a = static_cast<sf::Uint8>(255 * t.alpha);
                sf::Color boxColor = t.box.getFillColor();
                boxColor.a = a;
                t.box.setFillColor(boxColor);
                t.title.setFillColor(theme::withAlpha(theme::TEXT_WHITE, a));
                t.content.setFillColor(theme::withAlpha(theme::TEXT_SUBTLE, a));

                t.box.setOutlineThickness(1.f);
                t.box.setOutlineColor(theme::withAlpha(theme::OUTLINE_MID, a));

                window.draw(t.box);
                window.draw(t.title);
                window.draw(t.content);

                y -= spacing;
            }
        }
    private:
        sf::Font *_font = nullptr;
    };
}

#endif
