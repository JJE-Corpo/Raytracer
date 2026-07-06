//
// Created by jazema on 5/11/26.
//

#include "MaterialPanel.hpp"

#include <algorithm>

#include "../EventRouter.hpp"
#include "../Theme.hpp"
#include "../LayoutPen.hpp"
#include "../ViewportHelper.hpp"
#include "../../../common/Material.hpp"
#include "../../../common/MaterialLibrary.hpp"
#include "../../../common/scene/IPrimitive.hpp"
#include "../../../common/scene/ISceneObject.hpp"

namespace
{
    // Deliberately generous double-click window, matching the hierarchy panel:
    // a first (selection) click can stall the UI thread on a synchronous
    // viewport re-render, inflating the measured gap between the two clicks.
    constexpr int NAME_DOUBLE_CLICK_MS = 700;
    constexpr float NAME_FIELD_HEIGHT = 18.f;
}

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

        this->_nameField.setFont(font);
        this->_nameField.setCharacterSize(12);
        this->_nameField.onCommit = [this](const std::string &value)
        {
            this->commitName(value);
        };

        this->_font = font;
    }

    void MaterialPanel::update(const sf::Vector2i mouse)
    {
        if (!this->_materialModelSelector.enabled)
            return;

        // While the name is being edited the field owns the interaction.
        if (this->_nameField.active)
        {
            this->_nameField.update(mouse);
            return;
        }
        this->_nameHovered = this->_nameRect.contains(static_cast<float>(mouse.x), static_cast<float>(mouse.y));

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

        // Region used both to hit-test the double-click that opens the rename and
        // to place the edit field over the label. Kept a little wider than the
        // text so short names still offer a comfortable target.
        // min/max rather than std::clamp: if the panel is ever narrower than the
        // 90px minimum, clamp's lo>hi precondition would be violated (UB).
        const float textWidth = this->_materialName.getLocalBounds().width;
        const float fieldWidth = std::min(std::max(textWidth + 24.f, 90.f), width);
        this->_nameRect = sf::FloatRect(layout.x, layout.y - 2.f, fieldWidth, NAME_FIELD_HEIGHT);
        if (this->_nameField.active)
            this->_nameField.relayout(this->_nameRect.left, this->_nameRect.top, this->_nameRect.width, this->_nameRect.height);

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

        // A rebuild means the shown material may have changed; abandon any inline
        // rename in progress so it can't commit onto the wrong material.
        if (this->_nameField.active)
            this->_nameField.cancel();
        this->_namePendingClick = false;
        this->_material = nullptr;

        const auto *primitive = dynamic_cast<const IPrimitive *>(currentObject);

        this->_materialName.setString("(none)");

        if (!primitive)
            return;

        const Material *material = primitive->getMaterial();

        if (!material)
            return;

        this->_material = const_cast<Material *>(material);
        this->_materialName.setString(material->name);

        this->_materialModelSelector.setFont(this->_font);
        this->_materialModelSelector.setLabel("Model");
        this->_materialModelSelector.setOptions("Phong", "PBR");
        this->_materialModelSelector.setSelectedIndex(material->model == MaterialModel::PHONG ? 0 : 1);
        this->_materialModelSelector.onChange = [primitive, this](int index)
        {
            const MaterialModel model = index == 0 ? MaterialModel::PHONG : MaterialModel::PBR;
            const_cast<Material *>(primitive->getMaterial())->model = model;
            this->persistMaterial();
            this->rebuild(primitive);
        };
        this->_materialModelSelector.enabled = true;

        this->_baseColorPicker.enabled = true;
        this->_baseColorPicker.setColor(material->getBaseColor());
        this->_baseColorPicker.onChange = [primitive, this](const ColorF &color)
        {
            const_cast<Material *>(primitive->getMaterial())->baseColor = color;
            this->persistMaterial();
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
            slider.onChange            = [prop_key, primitive, this](float value)
            {
                const_cast<Material *>(primitive->getMaterial())->update<float>(prop_key, value);
                this->persistMaterial();
            };

            this->_materialSliders.push_back(slider);
        }
    }

    CursorType MaterialPanel::getCursor()
    {
        CursorType cursorType = CursorType::ARROW;

        if (!this->_materialModelSelector.enabled)
            return (CursorType::ARROW);
        if (this->_nameField.active)
            return (this->_nameField.getCursor());
        if (this->_nameHovered)
            return (CursorType::TEXT);
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
        // An open name editor must keep keyboard focus, like the hierarchy rename.
        if (this->_nameField.isCapturing())
            return (true);
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

        // While renaming, the editor owns the interaction: typing, Enter (commit),
        // Escape (cancel) and a left click outside the field (commit).
        if (this->_nameField.active)
            return (this->_nameField.handleEvent(event, mouse));

        std::vector<Component *> children = {&this->_baseColorPicker, &this->_materialModelSelector};
        for (auto &slider : this->_materialSliders)
            children.push_back(&slider);

        // The router serves the color-picker pop-up first while it is open,
        // so it consumes clicks that overlap the sliders underneath it.
        if (EventRouter::route(children, event, mouse) != nullptr)
            return (true);

        // Nothing else took the event: a double-click on the name label starts
        // an inline rename, the same gesture used for slider values.
        if (event.type == sf::Event::MouseButtonPressed
            && event.mouseButton.button == sf::Mouse::Left
            && this->_material
            && this->_nameRect.contains(static_cast<float>(mouse.x), static_cast<float>(mouse.y)))
        {
            const bool is_double_click = this->_namePendingClick
                && this->_nameClickClock.getElapsedTime().asMilliseconds() < NAME_DOUBLE_CLICK_MS;
            if (is_double_click)
            {
                this->_namePendingClick = false;
                this->beginNameEdit();
            }
            else
            {
                this->_namePendingClick = true;
                this->_nameClickClock.restart();
            }
            return (true);
        }
        return (false);
    }

    void MaterialPanel::beginNameEdit()
    {
        if (!this->_material)
            return;
        this->_nameField.begin(this->_material->name,
            this->_nameRect.left, this->_nameRect.top, this->_nameRect.width, this->_nameRect.height);
    }

    void MaterialPanel::commitName(const std::string &value)
    {
        if (!this->_material)
            return;

        const size_t first = value.find_first_not_of(" \t");
        const size_t last = value.find_last_not_of(" \t");
        const std::string trimmed = (first == std::string::npos) ? "" : value.substr(first, last - first + 1);

        // A blank name is meaningless; keep the previous one.
        if (trimmed.empty())
            return;

        const std::string previousName = this->_material->name;
        this->_material->name = trimmed;
        this->_materialName.setString(trimmed);

        // Rename in the market too: drop the old file before writing the new one
        // so a renamed material does not leave a stale entry behind.
        if (previousName != trimmed)
            MaterialLibrary::remove(previousName);
        this->persistMaterial();
    }

    void MaterialPanel::persistMaterial()
    {
        if (this->_material)
            MaterialLibrary::save(*this->_material);
    }

    void MaterialPanel::draw(sf::RenderTarget &target, sf::RenderStates states) const
    {
        // The label and its inline editor are mutually exclusive: while renaming
        // the field stands in for the static text.
        if (this->_nameField.active)
            target.draw(this->_nameField, states);
        else
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
