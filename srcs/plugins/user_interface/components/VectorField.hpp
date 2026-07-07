//
// Created by jazema on 5/11/26.
//

#ifndef VECTORFIELD_HPP
#define VECTORFIELD_HPP
#include <algorithm>
#include "TextField.hpp"
#include "../LayoutPen.hpp"
#include "../Component.hpp"
#include "../../../common/Axis.hpp"

namespace rc
{
    struct VectorField: Component
    {
        private:
            sf::Text _title;
            TextField x;
            TextField y;
            TextField z;

            static constexpr float LABEL_WIDTH = 60.0f;
            static constexpr float FIELD_HEIGHT = 20.0f;
            static constexpr float FIELD_GAP = 6.0f;
            const std::function<bool(const std::string&)> genericOnType = [&](const std::string &str)
            {
                return (Utils::isFloat(str));
            };
        public:
            std::function<bool(Axis axis, float)> onValidate = [&](Axis, float){
                return (false);
            };

            void setFont(sf::Font &font) override
            {
                this->_title.setFont(font);
                this->x.setFont(font);
                this->x.setCharacterSize(12);
                this->x.onType = this->genericOnType;
                this->x.onValidate = [&](const std::string &str)
                {
                    if (!Utils::isFloat(str))
                        return (false);
                    return (this->onValidate(Axis::X, std::stof(str)));
                };
                this->y.setFont(font);
                this->y.setCharacterSize(12);
                this->y.onType = this->genericOnType;
                this->y.onValidate = [&](const std::string &str)
                {
                    if (!Utils::isFloat(str))
                        return (false);
                    return (this->onValidate(Axis::Y, std::stof(str)));
                };
                this->z.setFont(font);
                this->z.setCharacterSize(12);
                this->z.onType = this->genericOnType;
                this->z.onValidate = [&](const std::string &str)
                {
                    if (!Utils::isFloat(str))
                        return (false);
                    return (this->onValidate(Axis::Z, std::stof(str)));
                };
            }

            void setLabel(const std::string &text)
            {
                this->_title.setString(text);
                this->_title.setCharacterSize(12);
                this->_title.setFillColor(theme::TEXT_WHITE);
            }

            void setValue(Vector3f value)
            {
                this->x.setValue(std::to_string(value.x));
                this->y.setValue(std::to_string(value.y));
                this->z.setValue(std::to_string(value.z));
            }

            void layout(float x, float y, float width)
            {
                LayoutPen layout{x, y};

                this->_title.setPosition(layout.x, layout.y);
                layout.next(8);

                float singleWidth = (width - FIELD_GAP * 2.f) / 3.f;
                float step = singleWidth + FIELD_GAP;
                this->x.layout(layout.x, layout.y, singleWidth, FIELD_HEIGHT);
                this->y.layout(layout.x + step, layout.y, singleWidth, FIELD_HEIGHT);
                this->z.layout(layout.x + step * 2.f, layout.y, singleWidth, FIELD_HEIGHT);
                this->x.enabled = true;
                this->y.enabled = true;
                this->z.enabled = true;
            }

            sf::FloatRect getBounds() const override
            {
                const sf::FloatRect a = this->x.getBounds();
                const sf::FloatRect b = this->z.getBounds();
                const float left = std::min(a.left, b.left);
                const float top = std::min(a.top, b.top);
                const float right = std::max(a.left + a.width, b.left + b.width);
                const float bottom = std::max(a.top + a.height, b.top + b.height);
                return sf::FloatRect(left, top, right - left, bottom - top);
            }

            bool isCapturing() const override
            {
                return (this->x.isCapturing() || this->y.isCapturing() || this->z.isCapturing());
            }

            bool handleEvent(const sf::Event &event, const sf::Vector2i mouse) override
            {
                bool consumed = this->x.handleEvent(event, mouse);
                consumed = this->y.handleEvent(event, mouse) || consumed;
                consumed = this->z.handleEvent(event, mouse) || consumed;
                return (consumed);
            }

            void update(sf::Vector2i mouse) override
            {
                this->x.update(mouse);
                this->y.update(mouse);
                this->z.update(mouse);
            }

            void draw(sf::RenderTarget &target, sf::RenderStates states) const override
            {
                target.draw(this->_title, states);
                target.draw(this->x, states);
                target.draw(this->y, states);
                target.draw(this->z, states);
            }

            CursorType getCursor() override
            {
                CursorType type = CursorType::ARROW;

                if (this->x.getCursor() != CursorType::ARROW)
                    type = this->x.getCursor();
                if (this->y.getCursor() != CursorType::ARROW)
                    type = this->y.getCursor();
                if (this->z.getCursor() != CursorType::ARROW)
                    type = this->z.getCursor();
                return type;
            }
    };
}

#endif
