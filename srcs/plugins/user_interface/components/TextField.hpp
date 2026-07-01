//
// Created by jazema on 5/1/26.
//

#ifndef TEXTFIELD_HPP
#define TEXTFIELD_HPP
#include <SFML/Graphics.hpp>
#include <functional>

#include "../Component.hpp"
#include "../Theme.hpp"

namespace rc
{
    struct TextField: Component
    {
        mutable sf::RectangleShape box;
        sf::Text text;

        mutable sf::RectangleShape caret;
        sf::Clock caretClock;

        std::string value;
        bool focused = false;

        std::function<bool(const std::string&)> onType = nullptr;
        std::function<bool(const std::string&)> onValidate = nullptr;

        bool caretVisible = true;
        size_t cursorPos = 0;

        size_t getCursorPosFromMouse(float mouseX) const
        {
            if (this->value.empty())
                return 0;

            float startX = this->text.getPosition().x;
            if (mouseX <= startX)
                return 0;

            size_t maxIndex = this->value.size();
            for (size_t i = 0; i < maxIndex; ++i)
            {
                float leftX = this->text.findCharacterPos(i).x;
                float rightX = this->text.findCharacterPos(i + 1).x;
                float midX = (leftX + rightX) * 0.5f;
                if (mouseX < midX)
                    return i;
            }
            return maxIndex;
        }

        void setFont(sf::Font &font) override
        {
            this->text.setFont(font);
            this->text.setCharacterSize(16);
            this->text.setFillColor(theme::TEXT_WHITE);
            this->caret.setSize({1.f, static_cast<float>(this->text.getCharacterSize())});
            this->caret.setFillColor(theme::TEXT_WHITE);
        }

        void setCharacterSize(int size)
        {
            this->text.setCharacterSize(size);
            this->caret.setSize({1.f, static_cast<float>(this->text.getCharacterSize())});
        }

        void layout(float x, float y, float w, float h)
        {
            this->box.setPosition(x, y);
            this->box.setSize({w, h});
            float padding = 8.f;
            float textY = y + (h - this->text.getCharacterSize()) / 2.f - 2.f;
            this->text.setPosition(x + padding, textY);
        }

        void setValue(const std::string &v)
        {
            this->value = v;
            this->cursorPos = this->value.size();
            this->text.setString(this->value);
        }

        void update(sf::Vector2i mouse) override
        {
            this->hovered = this->box.getGlobalBounds().contains(static_cast<float>(mouse.x), static_cast<float>(mouse.y));
            if (!this->enabled)
                return;
            if (sf::Mouse::isButtonPressed(sf::Mouse::Left))
            {
                bool wasFocused = this->focused;
                this->focused = this->box.getGlobalBounds().contains((float)mouse.x, (float)mouse.y);

                if (this->focused && !wasFocused)
                {
                    this->caretClock.restart();
                    this->caretVisible = true;
                }

                if (this->focused)
                {
                    this->cursorPos = this->getCursorPosFromMouse((float)mouse.x);
                    this->caretClock.restart();
                    this->caretVisible = true;
                }
            }
            if (this->caretClock.getElapsedTime().asSeconds() > 0.5f)
            {
                this->caretVisible = !this->caretVisible;
                this->caretClock.restart();
            }
        }

        void handleEvent(const sf::Event &event, const sf::Vector2i mouse) override
        {
            if (!this->focused || !this->enabled)
                return;

            (void)mouse;
            if (event.type == sf::Event::TextEntered)
            {
                if (event.text.unicode == 8) // backspace
                {
                    if (this->cursorPos > 0)
                    {
                        std::string newValue = this->value;
                        newValue.erase(this->cursorPos - 1, 1);
                        bool allowed = !this->onType || this->onType(newValue);
                        if (allowed)
                        {
                            this->value = std::move(newValue);
                            this->cursorPos--;
                            this->caretClock.restart();
                            this->caretVisible = true;
                        }
                    }
                }
                else if (event.text.unicode < 128 && std::isprint(event.text.unicode))
                {
                    char ch = static_cast<char>(event.text.unicode);
                    std::string newValue = this->value;
                    newValue.insert(this->cursorPos, 1, ch);
                    bool allowed = !this->onType || this->onType(newValue);
                    if (allowed)
                    {
                        this->value = std::move(newValue);
                        this->cursorPos++;
                        this->caretClock.restart();
                        this->caretVisible = true;
                    }
                }
                this->text.setString(this->value);
            }

            if (event.type == sf::Event::KeyPressed)
            {
                if (event.key.code == sf::Keyboard::Return)
                {
                    if (this->onValidate)
                    {
                        bool allowed = this->onValidate(this->value);
                        if (!allowed)
                            return;
                        this->focused = false;
                    }
                }

                if (event.key.code == sf::Keyboard::Left && this->cursorPos > 0)
                {
                    this->cursorPos--;
                    this->caretClock.restart();
                    this->caretVisible = true;
                }

                if (event.key.code == sf::Keyboard::Right && this->cursorPos < this->value.size())
                {
                    this->cursorPos++;
                    this->caretClock.restart();
                    this->caretVisible = true;
                }
            }

        }

        void draw(sf::RenderTarget &target, sf::RenderStates states) const override
        {
            this->box.setFillColor(!this->enabled ? theme::BG_DISABLED
                                     : this->focused ? theme::BG_CONTROL_HOVER
                                               : theme::BG_CONTROL);

            target.draw(this->box, states);
            target.draw(this->text, states);
            if (this->focused && this->caretVisible && this->enabled)
            {
                sf::Vector2f caretPos = this->text.findCharacterPos(this->cursorPos);

                this->caret.setPosition(
                    caretPos.x,
                    this->text.getPosition().y
                );
                target.draw(this->caret, states);
            }
        }

        CursorType getCursor() override
        {
            if (!this->enabled && this->hovered)
                return (CursorType::NOT_ALLOWED);
            if (this->hovered)
                return (CursorType::TEXT);
            return (CursorType::ARROW);
        }
    };
}

#endif
