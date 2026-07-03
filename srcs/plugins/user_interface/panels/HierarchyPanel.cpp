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

    // Double-click window for starting an inline rename. Deliberately larger
    // than a typical OS double-click (~500ms): the first click changes the
    // selection, which triggers a synchronous viewport re-render inside the UI
    // loop, so a second click landing during that render is only processed on
    // the next iteration and the measured gap between the two clicks is
    // inflated by the render time. The window must absorb that stall.
    constexpr int DOUBLE_CLICK_MS = 700;
}

namespace rc
{
    void HierarchyPanel::setFont(sf::Font &font)
    {
        this->_font = &font;
        this->_renameField.setFont(font);
        this->_renameField.setCharacterSize(12);
        this->_renameField.onValidate = [this](const std::string &)
        {
            this->commitRename();
            return (true);
        };
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
            y += ITEM_HEIGHT;
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
            const float buttonSize = 10.f;
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

            y += ITEM_HEIGHT;
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

        this->refreshHoverState();

        if (this->_renamingObject)
        {
            bool stillPresent = false;
            for (const auto &item : this->_items)
            {
                if (item.payload == this->_renamingObject)
                {
                    this->layoutRenameField(item.bounds, item.buttonBounds);
                    stillPresent = true;
                    break;
                }
            }
            if (!stillPresent)
                this->cancelRename();
        }

        finish();
    }

    // Re-applies the last known mouse position to the freshly-built item list.
    // buildItems() runs more than once per frame (update() then draw()), and
    // each run replaces _items wholesale, so hover can't just be set once by
    // update() - it has to be recomputed every time the list is rebuilt.
    void HierarchyPanel::refreshHoverState()
    {
        this->hovered = false;
        for (auto &item : this->_items)
        {
            item.hovered = item.selectable && item.bounds.contains(static_cast<sf::Vector2f>(this->_lastMouse));
            if (item.hovered)
                this->hovered = true;
        }
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

            const bool isRenamingThis = item.payload && item.payload == this->_renamingObject;

            if (!isRenamingThis)
            {
                sf::Text text;
                text.setFont(*this->_font);
                text.setCharacterSize(12);
                text.setFillColor(item.hidden ? theme::TEXT_DIM : theme::TEXT_MAIN);
                text.setString(item.label);
                text.setPosition({item.bounds.left + 8.f, item.bounds.top + 2.f});
                target.draw(text, states);
            }

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

        if (this->_renamingObject)
            target.draw(this->_renameField, states);
    }

    void HierarchyPanel::update(sf::Vector2i mouse)
    {
        this->_lastMouse = mouse;
        this->refreshHoverState();

        if (this->_renamingObject)
            this->_renameField.update(mouse);
    }

    bool HierarchyPanel::handleEvent(const sf::Event &event, const sf::Vector2i mouse)
    {
        if (!this->enabled)
            return (false);

        if (this->_renamingObject)
        {
            if (event.type == sf::Event::KeyPressed && event.key.code == sf::Keyboard::Escape)
            {
                this->cancelRename();
                return (true);
            }
            if (this->_renameField.handleEvent(event, mouse))
                return (true);
            // A left click outside the field commits and closes the editor,
            // mirroring ColorPicker's "click outside the popup" convention.
            if (event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Left)
            {
                this->commitRename();
                return (true);
            }
            return (false);
        }

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

            const bool is_double_click = item.payload == this->_lastClickedPayload
                && this->_clickClock.getElapsedTime().asMilliseconds() < DOUBLE_CLICK_MS;
            this->_lastClickedPayload = item.payload;
            this->_clickClock.restart();

            if (is_double_click && (item.type == ItemType::LIGHT || item.type == ItemType::PRIMITIVE))
            {
                this->beginRename(item);
                return (true);
            }

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

        // Clicked inside the panel but not on any row: clear the selection.
        const bool had_selection = this->_cameraSelected || !this->_selection.empty();
        this->_selection.clear();
        this->_cameraSelected = false;
        if (had_selection)
            this->_selectionChanged = true;
        return (true);
    }

    CursorType HierarchyPanel::getCursor()
    {
        if (!this->enabled)
            return (CursorType::ARROW);
        if (this->hovered)
            return (CursorType::HAND);
        return (CursorType::ARROW);
    }

    // Keeps keyboard focus on the rename field regardless of cursor position,
    // the same way an open ColorPicker popup captures events (see ColorPicker).
    bool HierarchyPanel::isCapturing() const
    {
        return (this->_renameField.isCapturing());
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

    void HierarchyPanel::beginRename(const Item &item)
    {
        const auto *object = static_cast<const ISceneObject *>(item.payload);
        if (!object)
            return;

        this->select(object, false);
        this->_selectionChanged = true;

        this->_renamingObject = object;
        this->_renameField.setValue(item.label);
        this->_renameField.focused = true;
        this->layoutRenameField(item.bounds, item.buttonBounds);

        // Start a fresh click count so that a single click on this same row
        // after the rename is committed/cancelled doesn't re-trigger rename.
        this->_lastClickedPayload = nullptr;
    }

    void HierarchyPanel::layoutRenameField(const sf::FloatRect &itemBounds, const sf::FloatRect &buttonBounds)
    {
        const float left = itemBounds.left + 2.f;
        const float right = buttonBounds.left - 4.f;
        const float width = std::max(20.f, right - left);
        this->_renameField.layout(left, itemBounds.top + 1.f, width, itemBounds.height - 2.f);
    }

    void HierarchyPanel::commitRename()
    {
        if (!this->_renamingObject)
            return;

        const std::string &value = this->_renameField.value;
        const size_t first = value.find_first_not_of(" \t");
        const size_t last = value.find_last_not_of(" \t");
        const std::string trimmed = (first == std::string::npos) ? "" : value.substr(first, last - first + 1);

        if (!trimmed.empty())
            const_cast<ISceneObject *>(this->_renamingObject)->setName(trimmed);

        this->_renamingObject = nullptr;
        this->_renameField.focused = false;
    }

    void HierarchyPanel::cancelRename()
    {
        this->_renamingObject = nullptr;
        this->_renameField.focused = false;
    }
}
