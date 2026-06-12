//
// Created by jazema on 5/11/26.
//

#ifndef VECTORFIELD_HPP
#define VECTORFIELD_HPP
#include "TextField.hpp"
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
                VerticalLayout layout{x, y};

                this->_title.setPosition(layout.x, layout.y);
                layout.next(8);

                float singleWidth = width / 3;
                this->x.layout(layout.x, layout.y, singleWidth, FIELD_HEIGHT);
                this->y.layout(layout.x + singleWidth, layout.y, singleWidth, FIELD_HEIGHT);
                this->z.layout(layout.x + singleWidth * 2, layout.y, singleWidth, FIELD_HEIGHT);
                this->x.enabled = true;
                this->y.enabled = true;
                this->z.enabled = true;
            }

            void handleEvent(const sf::Event &event, const sf::Vector2i mouse) override
            {
                this->x.handleEvent(event, mouse);
                this->y.handleEvent(event, mouse);
                this->z.handleEvent(event, mouse);
            }

            void update(sf::Vector2i mouse) override
            {
                this->x.update(mouse);
                this->y.update(mouse);
                this->z.update(mouse);
            }

            void draw(sf::RenderWindow &window) override
            {
                window.draw(this->_title);
                this->x.draw(window);
                this->y.draw(window);
                this->z.draw(window);
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
