//
// Created by jazema on 5/11/26.
//

#include "ObjectPanel.hpp"

#include "../EventRouter.hpp"
#include "../Theme.hpp"
#include "../LayoutPen.hpp"
#include "../../../common/Axis.hpp"
#include "../../../common/Material.hpp"
#include "../../../common/Utils.hpp"
#include "../../../common/scene/IScene.hpp"
#include "../../../common/scene/ILight.hpp"
#include "../../../common/scene/IPrimitive.hpp"
#include "../../../common/scene/ISceneObject.hpp"

namespace rc
{
    void ObjectPanel::setFont(sf::Font &font)
    {
        // this->_title.setFont(font);
        // this->_title.setCharacterSize(14);
        // this->_title.setFillColor(theme::TEXT_DIM);

        this->_lightColorPicker.setFont(font);
        this->_lightColorPicker.setLabel("Light color");

        this->_intensityLabel.setFont(font);
        this->_intensityLabel.setCharacterSize(12);
        this->_intensityLabel.setFillColor(theme::TEXT_DIM);
        this->_intensityLabel.setString("Light intensity");

        this->_lightIntensityField.setFont(font);
        this->_lightIntensityField.setCharacterSize(12);

        this->_vertexField.setLabel("Vertex");
        this->_vertexField.setFont(font);
        this->_vertexField.onValidate = [this](Axis axis, float value)
        {
            if (this->onVertexEdit)
                return (this->onVertexEdit(axis, value));
            return (false);
        };

        this->_positionField.setLabel("Position");
        this->_positionField.setFont(font);
        this->_rotationField.setLabel("Rotation");
        this->_rotationField.setFont(font);
        this->_scaleField.setLabel("Scale");
        this->_scaleField.setFont(font);

        this->_materialLabel.setFont(font);
        this->_materialLabel.setCharacterSize(12);
        this->_materialLabel.setFillColor(theme::TEXT_WHITE);
        this->_materialLabel.setString("Material");

        this->_materialDropdown.setFont(font);
        this->_materialDropdown.setPlaceholder("Select");

        this->_font = font;
    }

    void ObjectPanel::layout(float x, float y, float width)
    {
        LayoutPen layout{x, y};

        // this->_title.setPosition({layout.x, layout.y});
        // layout.next(24);

        if (this->_showVertexEditor)
        {
            this->_vertexField.layout(layout.x, layout.y, width);
            layout.next(32);
        }

        if (this->isLight)
        {
            this->_lightColorPicker.layout(layout.x, layout.y);
            layout.next(22);

            this->_intensityLabel.setPosition(layout.x, layout.y);
            layout.next(12);

            this->_lightIntensityField.layout(layout.x, layout.y, width, 20);
            layout.next(20);
        }

        if (this->isPrimitive)
        {
            this->_materialLabel.setPosition(layout.x, layout.y);
            this->_materialDropdown.layout(layout.x + 60, layout.y, width - 60.0f);
            layout.next(24);
        }

        for (auto &slider : this->_objectSliders)
        {
            slider.layout(layout.x, layout.y, width);
            layout.next(16);
        }

        this->_positionField.layout(layout.x, layout.y, width);
        layout.next(32);
        this->_rotationField.layout(layout.x, layout.y, width);
        layout.next(32);
        this->_scaleField.layout(layout.x, layout.y, width);
        layout.next(32);

        this->height = layout.y - y;
    }

    void ObjectPanel::setVertexEditor(bool visible, const Vector3f &value)
    {
        this->_showVertexEditor = visible;
        if (visible)
            this->_vertexField.setValue(value);
    }

    void ObjectPanel::update(sf::Vector2i mouse)
    {
        if (this->_showVertexEditor)
            this->_vertexField.update(mouse);
        if (this->isLight)
        {
            this->_lightColorPicker.update(mouse);
            this->_lightIntensityField.update(mouse);
        }
        if (this->isPrimitive)
            this->_materialDropdown.update(mouse);
        for (auto &slider : this->_objectSliders)
        {
            slider.update(mouse);
        }
        this->_positionField.update(mouse);
        this->_rotationField.update(mouse);
        this->_scaleField.update(mouse);
    }

    void ObjectPanel::rebuild(const ISceneObject *currentObject)
    {
        // this->_title.setString("Object : " + currentObject->getName());

        this->isLight = false;
        this->isPrimitive = false;

        this->_lightColorPicker.onChange = nullptr;
        this->_lightIntensityField.onType = nullptr;
        this->_lightIntensityField.onValidate = nullptr;
        this->_materialDropdown.onSelect = nullptr;
        this->_materialDropdown.enabled = false;
        this->_materialDropdown.open = false;

        this->_objectSliders.clear();

        const auto *asLight = dynamic_cast<const ILight *>(currentObject);
        const auto *asPrimitive = dynamic_cast<const IPrimitive *>(currentObject);
        if (asLight)
        {
            this->isLight = true;
            this->_lightColorPicker.onChange = [currentObject](const ColorF &color)
            {
                const auto *tmp = dynamic_cast<const ILight *>(currentObject);
                auto *light = const_cast<ILight *>(tmp);
                light->setColorF(color);
            };
            this->_lightIntensityField.enabled = true;
            this->_lightIntensityField.setValue(std::to_string(asLight->getIntensity()));
            this->_lightIntensityField.onType = [](const std::string &value)
            {
                if (!Utils::isFloat(value))
                    return (false);
                if (std::stof(value) < 0)
                    return (false);
                return (true);
            };
            this->_lightIntensityField.onValidate = [asLight](const std::string &value)
            {
                if (!Utils::isFloat(value))
                    return (false);
                float result = std::stof(value);
                if (result < 0)
                    result = 0;
                const_cast<ILight *>(asLight)->setIntensity(result);
                return (true);
            };
        }
        if (asPrimitive)
        {
            this->isPrimitive = true;
            std::vector<std::string> labels;
            labels.emplace_back("(none)");
            labels.emplace_back("New");
            for (const auto &material : this->_materials)
                labels.push_back(material->name);

            this->_materialDropdown.setOptions(labels);

            int selectedIndex = 0;
            if (asPrimitive->getMaterial() != nullptr)
            {
                const std::string &currentName = asPrimitive->getMaterial()->name;
                for (size_t i = 1; i < labels.size(); ++i)
                {
                    if (labels[i] == currentName)
                    {
                        selectedIndex = static_cast<int>(i);
                        break;
                    }
                }
            }

            this->_materialDropdown.setSelectedIndex(selectedIndex);
            this->_materialDropdown.onSelect = [this, asPrimitive](int index)
            {
                auto *primitive = const_cast<IPrimitive *>(asPrimitive);
                if (index <= 0)
                {
                    primitive->setMaterial(nullptr);
                    this->_materialChanged = true;
                    return;
                }

                if (index == 1)
                {
                    Material *material = this->scene->createMaterial();
                    this->setScene(this->scene); //pour mettre a jour la liste des materials;
                    primitive->setMaterial(material);
                    this->rebuild(primitive);
                    this->_materialChanged = true;
                    return;
                }

                const size_t materialIndex = static_cast<size_t>(index - 2);
                if (materialIndex < this->_materials.size())
                    primitive->setMaterial(this->_materials[materialIndex]);
                this->_materialChanged = true;
            };
            this->_materialDropdown.enabled = true;
            auto props = asPrimitive->getProperties();
            for (auto &[key, val] : props)
            {
                if (val.second != PropertyType::FLOAT)
                    continue;
                std::string _tmp = key;
                Slider slider;

                slider.setLabel(key);
                slider.setFont(this->_font);
                slider.setRange(0.0f, 100.0f);
                slider.setValue(std::stof(val.first));
                slider.onChange = [_tmp, asPrimitive](const float value)
                {
                    const_cast<IPrimitive *>(asPrimitive)->setPropertyFloat(_tmp, value);
                };
                this->_objectSliders.push_back(slider);
            }
        }
        // The editor works on the LOCAL (parent-relative) transform. For a
        // top-level object local == world, so un-grouped editing is unchanged;
        // for a child this edits its offset within the group and the flatten
        // pass (triggered by onSceneMutated) recomputes its world transform.
        auto *obj = const_cast<ISceneObject *>(currentObject);
        this->_positionField.setValue(currentObject->getLocalPosition());
        this->_positionField.onValidate = [this, obj](Axis axis, float value)
        {
            Vector3f result = obj->getLocalPosition();
            if (axis == Axis::X) result.x = value;
            if (axis == Axis::Y) result.y = value;
            if (axis == Axis::Z) result.z = value;
            obj->setLocalPosition(result);
            if (this->onSceneMutated)
                this->onSceneMutated();
            return (true);
        };
        this->_rotationField.setValue(currentObject->getLocalRotation());
        this->_rotationField.onValidate = [this, obj](Axis axis, float value)
        {
            Vector3f result = obj->getLocalRotation();
            if (axis == Axis::X) result.x = value;
            if (axis == Axis::Y) result.y = value;
            if (axis == Axis::Z) result.z = value;
            obj->setLocalRotation(result);
            if (this->onSceneMutated)
                this->onSceneMutated();
            return (true);
        };
        this->_scaleField.setValue(currentObject->getLocalScale());
        this->_scaleField.onValidate = [this, obj](Axis axis, float value)
        {
            Vector3f result = obj->getLocalScale();
            if (axis == Axis::X) result.x = value;
            if (axis == Axis::Y) result.y = value;
            if (axis == Axis::Z) result.z = value;
            obj->setLocalScale(result);
            if (this->onSceneMutated)
                this->onSceneMutated();
            return (true);
        };
    }

    bool ObjectPanel::isCapturing() const
    {
        if ((this->_showVertexEditor && this->_vertexField.isCapturing())
            || this->_lightColorPicker.isCapturing()
            || this->_materialDropdown.isCapturing()
            || this->_lightIntensityField.isCapturing()
            || this->_positionField.isCapturing()
            || this->_rotationField.isCapturing()
            || this->_scaleField.isCapturing())
            return (true);
        for (const auto &slider : this->_objectSliders)
            if (slider.isCapturing())
                return (true);
        return (false);
    }

    bool ObjectPanel::handleEvent(const sf::Event &event, const sf::Vector2i mouse)
    {
        std::vector<Component *> children;

        if (this->_showVertexEditor)
            children.push_back(&this->_vertexField);
        if (this->isLight)
        {
            children.push_back(&this->_lightColorPicker);
            children.push_back(&this->_lightIntensityField);
        }
        if (this->isPrimitive)
            children.push_back(&this->_materialDropdown);
        for (auto &slider : this->_objectSliders)
            children.push_back(&slider);
        children.push_back(&this->_positionField);
        children.push_back(&this->_rotationField);
        children.push_back(&this->_scaleField);

        // Open pop-ups (color picker, material dropdown) capture events and
        // are served first, so their clicks no longer leak to the sliders
        // or fields they overlap.
        return (EventRouter::route(children, event, mouse) != nullptr);
    }

    void ObjectPanel::draw(sf::RenderTarget &target, sf::RenderStates states) const
    {
        // target.draw(this->_title, states);

        if (this->_showVertexEditor)
            target.draw(this->_vertexField, states);

        for (auto &slider : this->_objectSliders)
        {
            target.draw(slider, states);
        }

        target.draw(this->_positionField, states);
        target.draw(this->_rotationField, states);
        target.draw(this->_scaleField, states);

        if (this->isLight)
        {
            target.draw(this->_intensityLabel, states);
            target.draw(this->_lightIntensityField, states);
            target.draw(this->_lightColorPicker, states);
        }

        if (this->isPrimitive)
        {
            target.draw(this->_materialLabel, states);
            target.draw(this->_materialDropdown, states);
        }
    }

    void ObjectPanel::drawOverlay(sf::RenderTarget &target, sf::RenderStates states) const
    {
        if (this->isLight)
            this->_lightColorPicker.drawOverlay(target, states);
        if (this->isPrimitive)
            this->_materialDropdown.drawOverlay(target, states);
    }

    void ObjectPanel::setScene(IScene *scene)
    {
        this->scene = scene;
        this->_materials.clear();
        for (const auto &entry : scene->getMaterials())
            this->_materials.push_back(&entry.second);
    }

    CursorType ObjectPanel::getCursor()
    {
        CursorType result = CursorType::ARROW;

        if (this->isLight)
        {
            if (result == CursorType::ARROW && this->_lightColorPicker.getCursor() != CursorType::ARROW)
                result = this->_lightColorPicker.getCursor();
            if (result == CursorType::ARROW && this->_lightIntensityField.getCursor() != CursorType::ARROW)
                result = this->_lightIntensityField.getCursor();
        }

        if (this->isPrimitive)
        {
            if (result == CursorType::ARROW && this->_materialDropdown.getCursor() != CursorType::ARROW)
                result = this->_materialDropdown.getCursor();
        }

        if (!this->_materialDropdown.open)
        {
            for (auto &slider : this->_objectSliders)
            {
                if (result == CursorType::ARROW && slider.getCursor() != CursorType::ARROW)
                    result = slider.getCursor();
            }
        }

        std::vector<Component *> fields = {&this->_positionField, &this->_rotationField, &this->_scaleField};
        if (this->_showVertexEditor)
            fields.push_back(&this->_vertexField);

        for (auto *field : fields)
        {
            if (result == CursorType::ARROW && field->getCursor() != CursorType::ARROW)
                result = field->getCursor();
        }

        return (result);
    }

    bool ObjectPanel::consumeMaterialChanged()
    {
        bool result = this->_materialChanged;
        this->_materialChanged = false;
        return (result);
    }
}
