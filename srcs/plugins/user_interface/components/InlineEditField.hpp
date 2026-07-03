//
// Created by jazema on 7/3/26.
//
// A TextField that stays hidden until begin() is called, then floats over a
// host region as an inline editor. It commits on Enter or a click outside the
// field, and cancels on Escape. This is the same "double-click a label to edit
// it" affordance the hierarchy uses for renaming objects, extracted so any
// component (e.g. a Slider value) can reuse it instead of re-implementing the
// commit / cancel / click-outside state machine.
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

        // Called with the final text when the edit is committed (Enter or a
        // click outside the field). Never called on cancel.
        std::function<void(const std::string &)> onCommit;

        // Called when the edit is abandoned (Escape or an external cancel()).
        std::function<void()> onCancel;

        void setFont(sf::Font &font) override
        {
            this->field.setFont(font);
        }

        void setCharacterSize(int size)
        {
            this->field.setCharacterSize(size);
        }

        // Restrict which intermediate strings the field accepts while typing
        // (e.g. numeric-only). Forwarded to the wrapped TextField's onType.
        void setValidator(std::function<bool(const std::string &)> validator)
        {
            this->field.onType = std::move(validator);
        }

        // Reveal the editor over the given rectangle, pre-filled with initial.
        // The Enter binding is (re)established here rather than in setFont so the
        // callback always points at this instance even after the owner has been
        // copied (Sliders live in a std::vector and are copied on push_back).
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

        // Reposition the open editor without touching its contents (the host may
        // re-layout every frame while the editor is active).
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

        // Keeps keyboard focus on the field regardless of cursor position while
        // it is open, mirroring an open color-picker popup.
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

            // A left click outside the field commits and closes the editor,
            // matching the hierarchy rename and color-picker popup convention.
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
