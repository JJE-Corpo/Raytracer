//
// Created by jazema on 5/11/26.
//

#include "MaterialPanel.hpp"

#include "../EventRouter.hpp"
#include "../Theme.hpp"
#include "../LayoutPen.hpp"
#include "../ViewportHelper.hpp"
#include "../../../common/Material.hpp"
#include "../../../common/scene/IPrimitive.hpp"
#include "../../../common/scene/ISceneObject.hpp"

namespace rc
{
    void MaterialPanel::setFont(sf::Font &font)
    {
        this->_baseColorPicker.setFont(font);
        this->_baseColorPicker.setLabel("Base color");
        this->_materialName.setFont(font);
        this->_materialName.setCharacterSize(12);
        this->_materialName.setFillColor(theme::TEXT_DIM);
        this->_materialName.setString("(none)");
        this->_font = font;
    }

    void MaterialPanel::update(const sf::Vector2i mouse)
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

    void MaterialPanel::layout(float x, float y, float width)
    {
        LayoutPen layout;

        layout.x = x;
        layout.y = y;

        this->_materialName.setPosition({layout.x, layout.y});
        layout.next(12);

        if (!this->_materialModelSelector.enabled)
        {
            this->height = layout.y - y;
            return;
        }
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

    void MaterialPanel::rebuild(const ISceneObject *currentObject)
    {
        this->_materialSliders.clear();
        this->_materialModelSelector.enabled = false;
        this->_baseColorPicker.enabled = false;

        const auto *primitive = dynamic_cast<const IPrimitive *>(currentObject);

        this->_materialName.setString("(none)");

        if (!primitive)
            return;

        const Material *material = primitive->getMaterial();

        if (!material)
            return;

        this->_materialName.setString(material->name);

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

    CursorType MaterialPanel::getCursor()
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

    bool MaterialPanel::isCapturing() const
    {
        if (this->_baseColorPicker.isCapturing())
            return (true);
        // A slider whose inline value editor is open must keep keyboard focus.
        for (const auto &slider : this->_materialSliders)
            if (slider.isCapturing())
                return (true);
        return (false);
    }

    bool MaterialPanel::handleEvent(const sf::Event &event, const sf::Vector2i mouse)
    {
        if (!this->_materialModelSelector.enabled)
            return (false);

        std::vector<Component *> children = {&this->_baseColorPicker, &this->_materialModelSelector};
        for (auto &slider : this->_materialSliders)
            children.push_back(&slider);

        // The router serves the color-picker pop-up first while it is open,
        // so it consumes clicks that overlap the sliders underneath it.
        return (EventRouter::route(children, event, mouse) != nullptr);
    }

    void MaterialPanel::draw(sf::RenderTarget &target, sf::RenderStates states) const
    {
        target.draw(this->_materialName, states);
        if (!this->_materialModelSelector.enabled)
            return;
        target.draw(this->_materialModelSelector, states);
        for (auto &slider : this->_materialSliders)
        {
            target.draw(slider, states);
        }
        target.draw(this->_baseColorPicker, states);
    }

    void MaterialPanel::drawOverlay(sf::RenderTarget &target, sf::RenderStates states) const
    {
        if (this->_materialModelSelector.enabled)
            this->_baseColorPicker.drawOverlay(target, states);
    }
}
