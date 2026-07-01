//
// Created by jazema on 5/11/26.
//

#ifndef MATERIALPANEL_HPP
#define MATERIALPANEL_HPP
#include "../ViewportHelper.hpp"
#include "../../../common/Material.hpp"
#include "../../../common/scene/IPrimitive.hpp"
#include "../../../common/scene/ISceneObject.hpp"
#include "../components/SegmentedControl.hpp"
#include "../components/Slider.hpp"

namespace rc
{
    struct MaterialPanel : Component
    {
        private:
            sf::Text _title;
            sf::Text _description;
            SegmentedControl _materialModelSelector;
            ColorPicker _baseColorPicker;
            std::vector<Slider> _materialSliders;

            sf::Font _font;
        public:
            float height = 0;

            void setFont(sf::Font &font) override
            {
                this->_title.setFont(font);
                this->_title.setCharacterSize(14);
                this->_title.setFillColor(theme::TEXT_DIM);
                this->_baseColorPicker.setFont(font);
                this->_baseColorPicker.setLabel("Base color");
                this->_description.setFont(font);
                this->_description.setCharacterSize(12);
                this->_description.setFillColor(theme::TEXT_DIM);
                this->_description.setString("(none)");
                this->_font = font;
            }

            void update(const sf::Vector2i mouse) override
            {
                if (!this->_materialModelSelector.enabled)
                    return;
                this->_baseColorPicker.update(mouse);
                if (this->_baseColorPicker.open)
                    return;
                this->_materialModelSelector.update(mouse);
                for (auto &slider : this->_materialSliders)
                {
                    slider.update(mouse);
                }
            }

            void layout(float x, float y, float width)
            {
                VerticalLayout layout;

                layout.x = x;
                layout.y = y;

                this->_title.setString("Material");
                this->_title.setPosition({layout.x, layout.y});
                layout.next(24);

                this->_description.setPosition({layout.x, layout.y});
                layout.next(12);

                if (!this->_materialModelSelector.enabled)
                    return;
                this->_baseColorPicker.layout(layout.x, layout.y);
                layout.next(24);

                this->_materialModelSelector.layout(layout.x, layout.y, width);
                layout.next(24);
                for (auto &slider : this->_materialSliders)
                {
                    slider.layout(layout.x, layout.y, width);
                    layout.next(24);
                }
                this->height = layout.y - y;
            }

            void rebuild(const ISceneObject *currentObject)
            {
                this->_materialSliders.clear();
                this->_materialModelSelector.enabled = false;
                this->_baseColorPicker.enabled = false;

                const auto *primitive = dynamic_cast<const IPrimitive *>(currentObject);

                this->_description.setString("(none)");

                if (!primitive)
                    return;

                const Material *material = primitive->getMaterial();

                if (!material)
                    return;

                this->_description.setString(material->name);

                this->_materialModelSelector.setFont(this->_font);
                this->_materialModelSelector.setLabel("Model");
                this->_materialModelSelector.setOptions("Phong", "PBR");
                this->_materialModelSelector.setSelectedIndex(material->model == MaterialModel::PHONG ? 0 : 1);
                this->_materialModelSelector.onChange = [primitive, this](int index)
                {
                    const MaterialModel model = index == 0 ? MaterialModel::PHONG : MaterialModel::PBR;
                    const_cast<Material *>(primitive->getMaterial())->model = model;
                    this->rebuild(primitive);
                };
                this->_materialModelSelector.enabled = true;

                this->_baseColorPicker.enabled = true;
                this->_baseColorPicker.setColor(material->getBaseColor());
                this->_baseColorPicker.onChange = [primitive, this](const ColorF &color)
                {
                    const_cast<Material *>(primitive->getMaterial())->baseColor = color;
                    this->rebuild(primitive);
                };

                const auto &props = material->getProperties();
                for (const auto &prop : props)
                {
                    const std::string &name = prop.first;
                    const auto &value = prop.second;

                    if (!ViewportHelper::isMaterialFloatSlider(name, material->model))
                        continue;

                    Slider slider;
                    slider.setFont(this->_font);
                    slider.setLabel(name);
                    slider.setRange(0.0f, 1.0f);

                    slider.setValue(value);

                    const std::string prop_key = name;
                    slider.onChange            = [prop_key, primitive](float value)
                    {
                        const_cast<Material *>(primitive->getMaterial())->update<float>(prop_key, value);
                    };

                    this->_materialSliders.push_back(slider);
                }
            }

            CursorType getCursor() override
            {
                CursorType cursorType = CursorType::ARROW;

                if (!this->_materialModelSelector.enabled)
                    return (CursorType::ARROW);
                if (this->_baseColorPicker.getCursor() != CursorType::ARROW)
                    cursorType = this->_baseColorPicker.getCursor();
                for (auto &slider : this->_materialSliders)
                {
                    if (cursorType == CursorType::ARROW && slider.getCursor() != CursorType::ARROW)
                        cursorType = slider.getCursor();
                }
                return (cursorType);
            }

            void handleEvent(const sf::Event &event, const sf::Vector2i mouse) override
            {
                if (!this->_materialModelSelector.enabled)
                    return;
                this->_baseColorPicker.handleEvent(event, mouse);
                this->_materialModelSelector.handleEvent(event, mouse);
                if (this->_baseColorPicker.open)
                    return;
                for (auto &slider : this->_materialSliders)
                    slider.handleEvent(event, mouse);
            }


            void draw(sf::RenderTarget &target, sf::RenderStates states) const override
            {
                target.draw(this->_title, states);
                target.draw(this->_description, states);
                if (!this->_materialModelSelector.enabled)
                    return;
                target.draw(this->_materialModelSelector, states);
                for (auto &slider : this->_materialSliders)
                {
                    target.draw(slider, states);
                }
                target.draw(this->_baseColorPicker, states);
            }
    };
}

#endif
