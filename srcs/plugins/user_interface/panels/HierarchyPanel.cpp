//
// Created by jazema on 5/7/26.
//

#include "HierarchyPanel.hpp"

#include <algorithm>
#include <cmath>

#include "../../../common/scene/IScene.hpp"
#include "../../../common/scene/IPrimitive.hpp"
#include "../../../common/scene/ILight.hpp"
#include "../Theme.hpp"

namespace
{
    constexpr float ITEM_HEIGHT = 18.f;
    constexpr float ITEM_INDENT = 12.f;
    constexpr float ITEM_SPACING = 6.f;
}

namespace rc
{
    void HierarchyPanel::setFont(sf::Font &font)
    {
        this->_font = &font;
    }

    void HierarchyPanel::setScene(IScene *scene)
    {
        this->_scene = scene;
    }

    void HierarchyPanel::layout(float x, float y, float width)
    {
        this->_originX = x;
        this->_originY = y;
        this->_width = width;
        this->buildItems();
    }

    float HierarchyPanel::getBottomY() const
    {
        return this->_bottomY;
    }

    const std::vector<const ISceneObject *> &HierarchyPanel::getSelection() const
    {
        return (this->_selection);
    }

    bool HierarchyPanel::isCameraSelected() const
    {
        return (this->_cameraSelected);
    }

    void HierarchyPanel::applyViewportSelection(const std::vector<const ISceneObject *> &selection)
    {
        this->_selection = selection;
        this->_cameraSelected = false;
    }

    bool HierarchyPanel::consumeSelectionChanged()
    {
        bool changed = this->_selectionChanged;
        this->_selectionChanged = false;
        return changed;
    }

    bool HierarchyPanel::isSelected(const ISceneObject *object) const
    {
        return (std::find(this->_selection.begin(), this->_selection.end(), object) != this->_selection.end());
    }

    void HierarchyPanel::buildItems()
    {
        this->_items.clear();

        float y = this->_originY;

        auto finish = [&]()
        {
            const float height = std::max(0.f, y - this->_originY);
            this->_bottomY = this->_originY + height;
            this->height = height;
        };

        if (!this->_scene)
        {
            Item empty;
            empty.label = "No scene loaded";
            empty.depth = 0;
            empty.type = ItemType::GROUP;
            empty.selectable = false;
            empty.selected = false;
            empty.hovered = false;
            empty.bounds = {this->_originX, y, this->_width, ITEM_HEIGHT};
            this->_items.push_back(empty);
            y += ITEM_HEIGHT + ITEM_SPACING;
            finish();
            return;
        }

        auto push_item = [&](const std::string &label, int depth, ItemType type, const void *payload, bool selectable, bool selected)
        {
            const float indent = ITEM_INDENT * static_cast<float>(depth);
            float itemX = this->_originX + indent;
            float itemW = std::max(0.f, this->_width - indent);

            Item item;
            item.label = label;
            item.depth = depth;
            item.type = type;
            item.payload = payload;
            item.selectable = selectable;
            item.selected = selected;
            item.hovered = false;
            item.bounds = {itemX, y, itemW, ITEM_HEIGHT};
            const float buttonSize = 14.f;
            item.buttonBounds = {item.bounds.left + item.bounds.width - buttonSize - 6.f, item.bounds.top + (item.bounds.height - buttonSize) / 2.f, buttonSize, buttonSize};
            item.hidden = false;
            if (payload)
            {
                if (type == ItemType::PRIMITIVE)
                    item.hidden = static_cast<const IPrimitive *>(payload)->isHidden();
                else if (type == ItemType::LIGHT)
                    item.hidden = static_cast<const ILight *>(payload)->isHidden();
            }
            this->_items.push_back(item);

            y += ITEM_HEIGHT + ITEM_SPACING;
        };

        push_item("Camera", 0, ItemType::CAMERA, &this->_scene->getCamera(), true, this->_cameraSelected);

        // push_item("Lights", 0, ItemType::GROUP, nullptr, false, false);
        const auto &lights = this->_scene->getLights();
        for (auto light : lights)
        {
            bool selected = (light && this->isSelected(light));
            push_item(light->getName(), 0, ItemType::LIGHT, light, true, selected);
        }

        // push_item("Primitives", 0, ItemType::GROUP, nullptr, false, false);
        const auto &primitives = this->_scene->getPrimitives();
        for (auto primitive : primitives)
        {
            std::string label = primitive->getName();
            bool selected = (isSelected(primitive));
            push_item(label, 0, ItemType::PRIMITIVE, primitive, true, selected);
        }

        finish();
    }

    void HierarchyPanel::draw(sf::RenderTarget &target, sf::RenderStates states) const
    {
        if (!this->_font)
            return;

        for (const auto &item : this->_items)
        {
            sf::RectangleShape bg;
            bg.setPosition({item.bounds.left, item.bounds.top});
            bg.setSize({item.bounds.width, item.bounds.height});

            if (item.selected)
                bg.setFillColor(theme::SELECTION_BG);
            else if (item.hovered)
                bg.setFillColor(theme::BG_HOVER);
            else
                bg.setFillColor(theme::BG_ITEM);

            target.draw(bg, states);

            if (item.selected)
            {
                sf::RectangleShape accent;
                accent.setPosition({item.bounds.left, item.bounds.top});
                accent.setSize({3.f, item.bounds.height});
                accent.setFillColor(theme::ACCENT);
                target.draw(accent, states);
            }

            sf::Text text;
            text.setFont(*this->_font);
            text.setCharacterSize(12);
            text.setFillColor(item.hidden ? theme::TEXT_DIM : theme::TEXT_MAIN);
            text.setString(item.label);
            text.setPosition({item.bounds.left + 8.f, item.bounds.top + 2.f});
            target.draw(text, states);

            if (item.type == ItemType::PRIMITIVE || item.type == ItemType::LIGHT)
            {
                sf::RectangleShape btn;
                btn.setPosition({item.buttonBounds.left, item.buttonBounds.top});
                btn.setSize({item.buttonBounds.width, item.buttonBounds.height});
                btn.setFillColor(item.hidden ? theme::TEXT_DIM : theme::BG_ITEM);
                btn.setOutlineColor(theme::TEXT_DIM);
                btn.setOutlineThickness(1.f);
                target.draw(btn, states);
            }
        }
    }

    void HierarchyPanel::update(sf::Vector2i mouse)
    {
        this->hovered = false;
        for (auto &item : this->_items)
        {
            item.hovered = item.selectable && item.bounds.contains(static_cast<sf::Vector2f>(mouse));
            if (item.hovered)
                this->hovered = true;
        }
    }

    bool HierarchyPanel::handleEvent(const sf::Event &event, const sf::Vector2i mouse)
    {
        if (!this->enabled)
            return (false);
        if (event.type != sf::Event::MouseButtonPressed)
            return (false);
        if (event.mouseButton.button != sf::Mouse::Left)
            return (false);

        bool ctrl_pressed = sf::Keyboard::isKeyPressed(sf::Keyboard::LControl) || sf::Keyboard::isKeyPressed(sf::Keyboard::RControl);

        for (const auto &item : this->_items)
        {
            if ((item.type == ItemType::PRIMITIVE || item.type == ItemType::LIGHT) && item.buttonBounds.contains(static_cast<sf::Vector2f>(mouse)))
            {
                if (!this->_onItemHideRequest)
                    return (true);
                const ISceneObject *asObject = static_cast<const ISceneObject *>(item.payload);
                if (this->isSelected(asObject))
                {
                    for (auto *object : this->_selection)
                    {
                        this->_onItemHideRequest(object);
                    }
                    return (true);
                }
                this->_onItemHideRequest(asObject);
                return (true);
            }
            if (!item.selectable)
                continue;
            if (!item.bounds.contains(static_cast<sf::Vector2f>(mouse)))
                continue;
            switch (item.type)
            {
                case ItemType::CAMERA:
                    this->selectCamera();
                    break;
                default:
                    this->select(static_cast<const ISceneObject *>(item.payload), ctrl_pressed);
                    break;
            }
            this->_selectionChanged = true;
            return (true);
        }
        return (false);
    }

    CursorType HierarchyPanel::getCursor()
    {
        if (!this->enabled)
            return (CursorType::ARROW);
        if (this->hovered)
            return (CursorType::HAND);
        return (CursorType::ARROW);
    }

    void HierarchyPanel::select(const ISceneObject *object, bool ctrlPressed)
    {
        if (!object)
            return;

        this->_cameraSelected = false;

        if (ctrlPressed)
        {
            auto it = std::find(this->_selection.begin(), this->_selection.end(), object);
            if (it != this->_selection.end())
                this->_selection.erase(it);
            else
                this->_selection.push_back(object);
            return;
        }
        this->_selection.clear();
        this->_selection.push_back(object);
    }

    void HierarchyPanel::selectCamera()
    {
        this->_selection.clear();
        this->_cameraSelected = true;
    }
}
