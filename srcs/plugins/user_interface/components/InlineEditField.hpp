//
// Created by jazema on 7/3/26.
//

#ifndef INLINEEDITFIELD_HPP
#define INLINEEDITFIELD_HPP

#include <functional>
#include <SFML/Graphics.hpp>

#include "TextField.hpp"
#include "../Component.hpp"

namespace rc
{
    struct InlineEditField : Component
    {
        TextField field;
        bool active = false;

        std::function<void(const std::string &)> onCommit;

        std::function<void()> onCancel;

        void setFont(sf::Font &font) override
        {
            this->field.setFont(font);
        }

        void setCharacterSize(int size)
        {
            this->field.setCharacterSize(size);
        }

        void setValidator(std::function<bool(const std::string &)> validator)
        {
            this->field.onType = std::move(validator);
        }

        void begin(const std::string &initial, float x, float y, float w, float h)
        {
            this->field.onValidate = [this](const std::string &)
            {
                this->commit();
                return (true);
            };
            this->field.setValue(initial);
            this->field.layout(x, y, w, h);
            this->field.focused = true;
            this->active = true;
        }

        void relayout(float x, float y, float w, float h)
        {
            this->field.layout(x, y, w, h);
        }

        void commit()
        {
            if (!this->active)
                return;
            this->active = false;
            this->field.focused = false;
            if (this->onCommit)
                this->onCommit(this->field.value);
        }

        void cancel()
        {
            if (!this->active)
                return;
            this->active = false;
            this->field.focused = false;
            if (this->onCancel)
                this->onCancel();
        }

        void update(sf::Vector2i mouse) override
        {
            if (this->active)
                this->field.update(mouse);
        }

        bool isCapturing() const override
        {
            return (this->active && this->field.isCapturing());
        }

        sf::FloatRect getBounds() const override
        {
            return (this->active ? this->field.getBounds() : sf::FloatRect());
        }

        bool handleEvent(const sf::Event &event, const sf::Vector2i mouse) override
        {
            if (!this->active)
                return (false);

            if (event.type == sf::Event::KeyPressed && event.key.code == sf::Keyboard::Escape)
            {
                this->cancel();
                return (true);
            }

            if (this->field.handleEvent(event, mouse))
                return (true);

            if (event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Left)
            {
                this->commit();
                return (true);
            }
            return (false);
        }

        void draw(sf::RenderTarget &target, sf::RenderStates states) const override
        {
            if (this->active)
                target.draw(this->field, states);
        }

        CursorType getCursor() override
        {
            return (this->active ? this->field.getCursor() : CursorType::ARROW);
        }
    };
}

#endif
