//
// Created by jazema on 5/7/26.
//

#include "HierarchyPanel.hpp"

#include <algorithm>
#include <cmath>
#include <functional>

#include "../../../common/scene/IScene.hpp"
#include "../../../common/scene/IPrimitive.hpp"
#include "../../../common/scene/ILight.hpp"
#include "../Theme.hpp"

namespace
{
    constexpr float ITEM_HEIGHT = 18.f;
    constexpr float ITEM_INDENT = 12.f;

    constexpr int DOUBLE_CLICK_MS = 700;

    constexpr float DRAG_THRESHOLD = 4.f;

    sf::FloatRect renameFieldRect(const sf::FloatRect &itemBounds, const sf::FloatRect &buttonBounds)
    {
        const float left = itemBounds.left + 2.f;
        const float right = buttonBounds.left - 4.f;
        const float width = std::max(20.f, right - left);
        return sf::FloatRect(left, itemBounds.top + 1.f, width, itemBounds.height - 2.f);
    }
}

namespace rc
{
    void HierarchyPanel::setFont(sf::Font &font)
    {
        this->_font = &font;
        this->_renameField.setFont(font);
        this->_renameField.setCharacterSize(12);
        this->_renameField.onCommit = [this](const std::string &value)
        {
            this->commitRename(value);
        };
        this->_renameField.onCancel = [this]()
        {
            this->_renamingObject = nullptr;
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
        this->_selectionAnchor = selection.empty() ? nullptr : selection.back();
        this->_selectionLead = this->_selectionAnchor;
    }

    bool HierarchyPanel::consumeSelectionChanged()
    {
        bool changed = this->_selectionChanged;
        this->_selectionChanged = false;
        return changed;
    }

    const ISceneObject *HierarchyPanel::consumeContextMenuRequest()
    {
        const ISceneObject *request = this->_contextMenuRequest;
        this->_contextMenuRequest = nullptr;
        return (request);
    }

    bool HierarchyPanel::isSelected(const ISceneObject *object) const
    {
        return (std::find(this->_selection.begin(), this->_selection.end(), object) != this->_selection.end());
    }

    bool HierarchyPanel::isExpanded(const ISceneObject *group) const
    {
        return (this->_collapsed.find(group) == this->_collapsed.end());
    }

    bool HierarchyPanel::isAncestorOf(const ISceneObject *ancestor, const ISceneObject *node) const
    {
        for (const ISceneObject *p = node; p != nullptr; p = p->getParent())
            if (p == ancestor)
                return (true);
        return (false);
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
            empty.bounds = {this->_originX, y, this->_width, ITEM_HEIGHT};
            this->_items.push_back(empty);
            y += ITEM_HEIGHT;
            finish();
            return;
        }

        auto push_item = [&](const std::string &label, int depth, ItemType type, const ISceneObject *object, bool selectable)
        {
            const float indent = ITEM_INDENT * static_cast<float>(depth);
            float itemX = this->_originX + indent;
            float itemW = std::max(0.f, this->_width - indent);

            Item item;
            item.label = label;
            item.depth = depth;
            item.type = type;
            item.object = object;
            item.selectable = selectable;
            item.selected = (type == ItemType::CAMERA) ? this->_cameraSelected : (object && this->isSelected(object));
            item.hovered = false;
            item.bounds = {itemX, y, itemW, ITEM_HEIGHT};
            const float buttonSize = 10.f;
            const float buttonY = item.bounds.top + (item.bounds.height - buttonSize) / 2.f;
            item.deleteButtonBounds = {item.bounds.left + item.bounds.width - buttonSize - 6.f, buttonY, buttonSize, buttonSize};
            item.buttonBounds = {item.deleteButtonBounds.left - buttonSize - 6.f, buttonY, buttonSize, buttonSize};
            item.hidden = (object != nullptr) && object->isHidden();
            item.expandable = (type == ItemType::GROUP);
            if (item.expandable)
            {
                item.expanded = this->isExpanded(object);
                const float ts = 10.f;
                item.toggleBounds = {item.bounds.left + 2.f, item.bounds.top + (item.bounds.height - ts) / 2.f, ts, ts};
            }
            this->_items.push_back(item);

            y += ITEM_HEIGHT;
        };

        push_item("Camera", 0, ItemType::CAMERA, nullptr, true);

        std::function<void(const ISceneObject *, int)> addRows = [&](const ISceneObject *object, int depth)
        {
            if (!object)
                return;
            ItemType type = ItemType::PRIMITIVE;
            switch (object->getObjectType())
            {
                case ObjectType::LIGHT: type = ItemType::LIGHT; break;
                case ObjectType::GROUP: type = ItemType::GROUP; break;
                default: type = ItemType::PRIMITIVE; break;
            }
            push_item(object->getName(), depth, type, object, true);
            if (object->getObjectType() == ObjectType::GROUP && this->isExpanded(object))
                for (const ISceneObject *child : object->getChildren())
                    addRows(child, depth + 1);
        };
        for (const ISceneObject *root : this->_scene->getRoots())
            addRows(root, 0);

        this->refreshHoverState();

        if (this->_renamingObject)
        {
            bool stillPresent = false;
            for (const auto &item : this->_items)
            {
                if (item.object == this->_renamingObject)
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

            // Expand/collapse triangle for group rows.
            if (item.expandable)
            {
                const sf::FloatRect &t = item.toggleBounds;
                sf::ConvexShape tri;
                tri.setPointCount(3);
                if (item.expanded)
                {
                    tri.setPoint(0, {t.left, t.top + 1.f});
                    tri.setPoint(1, {t.left + t.width, t.top + 1.f});
                    tri.setPoint(2, {t.left + t.width / 2.f, t.top + t.height - 1.f});
                }
                else
                {
                    tri.setPoint(0, {t.left + 1.f, t.top});
                    tri.setPoint(1, {t.left + 1.f, t.top + t.height});
                    tri.setPoint(2, {t.left + t.width - 1.f, t.top + t.height / 2.f});
                }
                tri.setFillColor(theme::TEXT_DIM);
                target.draw(tri, states);
            }

            const bool isRenamingThis = item.object && item.object == this->_renamingObject;

            if (!isRenamingThis)
            {
                sf::Text text;
                text.setFont(*this->_font);
                text.setCharacterSize(12);
                text.setFillColor(item.hidden ? theme::TEXT_DIM : theme::TEXT_MAIN);
                text.setString(item.label);
                const float labelX = item.expandable ? item.bounds.left + 16.f : item.bounds.left + 8.f;
                text.setPosition({labelX, item.bounds.top + 2.f});
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

            if (item.object && (item.hovered || item.selected))
            {
                const sf::FloatRect &d = item.deleteButtonBounds;
                const sf::Vector2f center(d.left + d.width / 2.f, d.top + d.height / 2.f);
                const sf::Color color = item.hovered ? sf::Color(230, 110, 110) : sf::Color(170, 90, 90);
                for (float angle : {45.f, -45.f})
                {
                    sf::RectangleShape bar({d.width, 2.f});
                    bar.setOrigin(d.width / 2.f, 1.f);
                    bar.setPosition(center);
                    bar.setRotation(angle);
                    bar.setFillColor(color);
                    target.draw(bar, states);
                }
            }
        }

        // Drop indicator while dragging.
        if (this->_dragActive && this->_dropMode != DropMode::NONE)
        {
            if (this->_dropMode == DropMode::INTO && this->_dropRow >= 0
                && this->_dropRow < static_cast<int>(this->_items.size()))
            {
                const sf::FloatRect &b = this->_items[this->_dropRow].bounds;
                sf::RectangleShape outline;
                outline.setPosition({b.left, b.top});
                outline.setSize({b.width, b.height});
                outline.setFillColor(sf::Color::Transparent);
                outline.setOutlineColor(theme::ACCENT);
                outline.setOutlineThickness(-2.f);
                target.draw(outline, states);
            }
            else if (this->_dropMode == DropMode::SIBLING)
            {
                sf::RectangleShape line;
                line.setPosition({this->_dropLineX, this->_dropLineY - 1.f});
                line.setSize({this->_dropLineW, 2.f});
                line.setFillColor(theme::ACCENT);
                target.draw(line, states);
            }
        }

        if (this->_renamingObject)
            target.draw(this->_renameField, states);
    }

    void HierarchyPanel::update(sf::Vector2i mouse)
    {
        this->_lastMouse = mouse;
        this->refreshHoverState();

        if (this->_dragActive)
            this->updateDropTarget(mouse);

        if (this->_renamingObject)
            this->_renameField.update(mouse);
    }

    void HierarchyPanel::setSiblingDrop(const ISceneObject *parent, int index, float lineX, float lineY, float lineW)
    {
        this->_dropMode = DropMode::SIBLING;
        this->_dropParent = parent;
        this->_dropIndex = index;
        this->_dropRow = -1;
        this->_dropLineX = lineX;
        this->_dropLineY = lineY;
        this->_dropLineW = lineW;
    }

    int HierarchyPanel::siblingIndexOf(const ISceneObject *parent, const ISceneObject *ref) const
    {
        static const std::vector<ISceneObject *> none;
        const std::vector<ISceneObject *> &siblings =
            parent ? parent->getChildren() : (this->_scene ? this->_scene->getRoots() : none);
        int index = 0;
        for (ISceneObject *s : siblings)
        {
            if (s == ref)
                break;
            if (s == this->_dragObject)
                continue;
            ++index;
        }
        return index;
    }

    void HierarchyPanel::updateDropTarget(sf::Vector2i mouse)
    {
        this->_dropMode = DropMode::NONE;
        this->_dropRow = -1;
        this->_dropParent = nullptr;
        this->_dropIndex = -1;
        if (!this->_dragObject || !this->_scene)
            return;

        const sf::Vector2f m(static_cast<float>(mouse.x), static_cast<float>(mouse.y));
        int hoveredRow = -1;
        for (std::size_t i = 0; i < this->_items.size(); ++i)
        {
            const Item &it = this->_items[i];
            if (m.y >= it.bounds.top && m.y < it.bounds.top + it.bounds.height)
            {
                hoveredRow = static_cast<int>(i);
                break;
            }
        }

        if (hoveredRow < 0)
        {
            this->setSiblingDrop(nullptr, static_cast<int>(this->_scene->getRoots().size()),
                this->_originX, this->_bottomY, this->_width);
            return;
        }

        const Item &target = this->_items[hoveredRow];

        if (target.type == ItemType::CAMERA)
        {
            this->setSiblingDrop(nullptr, 0, this->_originX,
                target.bounds.top + target.bounds.height, this->_width);
            return;
        }

        const ISceneObject *targetObj = target.object;

        if (!targetObj || targetObj == this->_dragObject || this->isAncestorOf(this->_dragObject, targetObj))
        {
            this->_dropMode = DropMode::NONE;
            return;
        }

        const float f = (m.y - target.bounds.top) / target.bounds.height;
        const bool isGroup = (target.type == ItemType::GROUP);

        if (isGroup && f > 0.30f && f < 0.70f)
        {
            this->_dropMode = DropMode::INTO;
            this->_dropParent = targetObj;
            this->_dropRow = hoveredRow;
            this->_dropIndex = -1; // append inside the group
            return;
        }

        const ISceneObject *parent = targetObj->getParent();
        if (f < 0.5f)
        {
            this->setSiblingDrop(parent, this->siblingIndexOf(parent, targetObj),
                target.bounds.left, target.bounds.top, target.bounds.width);
        }
        else if (isGroup && target.expanded && !targetObj->getChildren().empty()
                 && hoveredRow + 1 < static_cast<int>(this->_items.size()))
        {
            const sf::FloatRect &childBounds = this->_items[hoveredRow + 1].bounds;
            this->setSiblingDrop(targetObj, 0, childBounds.left,
                target.bounds.top + target.bounds.height, childBounds.width);
        }
        else
        {
            this->setSiblingDrop(parent, this->siblingIndexOf(parent, targetObj) + 1,
                target.bounds.left, target.bounds.top + target.bounds.height, target.bounds.width);
        }
    }

    void HierarchyPanel::commitDrop()
    {
        if (this->_dropMode == DropMode::NONE || !this->_dragObject || !this->_onReparentRequest)
            return;

        const ISceneObject *child = this->_dragObject;
        const ISceneObject *newParent = this->_dropParent;

        if (newParent && (newParent == child || this->isAncestorOf(child, newParent)))
            return;

        this->_onReparentRequest(child, newParent, this->_dropIndex);
        this->_selectionChanged = true;
    }

    bool HierarchyPanel::handleEvent(const sf::Event &event, const sf::Vector2i mouse)
    {
        if (!this->enabled)
            return (false);

        if (this->_renamingObject)
        {
            return (this->_renameField.handleEvent(event, mouse));
        }

        if (event.type == sf::Event::KeyPressed
            && (event.key.code == sf::Keyboard::Up || event.key.code == sf::Keyboard::Down))
        {
            const bool ctrl_pressed = sf::Keyboard::isKeyPressed(sf::Keyboard::LControl)
                || sf::Keyboard::isKeyPressed(sf::Keyboard::RControl);
            const bool shift_pressed = sf::Keyboard::isKeyPressed(sf::Keyboard::LShift)
                || sf::Keyboard::isKeyPressed(sf::Keyboard::RShift);
            this->moveSelection(event.key.code == sf::Keyboard::Down ? 1 : -1, ctrl_pressed, shift_pressed);
            return (true);
        }

        if (event.type == sf::Event::KeyPressed && event.key.code == sf::Keyboard::Delete)
        {
            const bool had_selection = !this->_selection.empty();
            this->deleteSelection();
            return (had_selection);
        }

        if (event.type == sf::Event::MouseMoved)
        {
            if (this->_dragObject && !sf::Mouse::isButtonPressed(sf::Mouse::Left))
            {
                this->_dragObject = nullptr;
                this->_dragActive = false;
                this->_dropMode = DropMode::NONE;
                this->_dropRow = -1;
                this->_dropParent = nullptr;
                return (false);
            }
            if (this->_dragObject && !this->_dragActive)
            {
                const float dx = static_cast<float>(mouse.x - this->_dragStartMouse.x);
                const float dy = static_cast<float>(mouse.y - this->_dragStartMouse.y);
                if (dx * dx + dy * dy > DRAG_THRESHOLD * DRAG_THRESHOLD)
                    this->_dragActive = true;
            }
            if (this->_dragActive)
            {
                this->updateDropTarget(mouse);
                return (true);
            }
            return (false);
        }

        if (event.type == sf::Event::MouseButtonReleased && event.mouseButton.button == sf::Mouse::Left)
        {
            const bool wasDragging = this->_dragActive;
            if (wasDragging)
                this->commitDrop();
            this->_dragObject = nullptr;
            this->_dragActive = false;
            this->_dropMode = DropMode::NONE;
            this->_dropRow = -1;
            this->_dropParent = nullptr;
            return (wasDragging);
        }

        if (event.type != sf::Event::MouseButtonPressed)
            return (false);

        if (event.mouseButton.button == sf::Mouse::Right)
        {
            for (const auto &item : this->_items)
            {
                if (!item.object || !item.bounds.contains(static_cast<sf::Vector2f>(mouse)))
                    continue;
                if (!this->isSelected(item.object))
                {
                    this->select(item.object, false);
                    this->_selectionChanged = true;
                }
                this->_contextMenuRequest = item.object;
                return (true);
            }
            return (false);
        }

        if (event.mouseButton.button != sf::Mouse::Left)
            return (false);

        bool ctrl_pressed = sf::Keyboard::isKeyPressed(sf::Keyboard::LControl) || sf::Keyboard::isKeyPressed(sf::Keyboard::RControl);
        bool shift_pressed = sf::Keyboard::isKeyPressed(sf::Keyboard::LShift) || sf::Keyboard::isKeyPressed(sf::Keyboard::RShift);

        for (const auto &item : this->_items)
        {
            if (item.expandable && item.object && item.toggleBounds.contains(static_cast<sf::Vector2f>(mouse)))
            {
                if (this->isExpanded(item.object))
                    this->_collapsed.insert(item.object);
                else
                    this->_collapsed.erase(item.object);
                return (true);
            }
            if ((item.type == ItemType::PRIMITIVE || item.type == ItemType::LIGHT) && item.buttonBounds.contains(static_cast<sf::Vector2f>(mouse)))
            {
                if (!this->_onItemHideRequest || !item.object)
                    return (true);
                if (this->isSelected(item.object))
                {
                    for (auto *object : this->_selection)
                        this->_onItemHideRequest(object);
                    return (true);
                }
                this->_onItemHideRequest(item.object);
                return (true);
            }
            if (item.object && item.deleteButtonBounds.contains(static_cast<sf::Vector2f>(mouse)))
            {
                if (this->isSelected(item.object))
                    this->deleteObjects(this->_selection);
                else
                    this->deleteObjects({item.object});
                return (true);
            }
            if (!item.selectable)
                continue;
            if (!item.bounds.contains(static_cast<sf::Vector2f>(mouse)))
                continue;

            if (item.object)
            {
                this->_dragObject = item.object;
                this->_dragStartMouse = mouse;
                this->_dragActive = false;
            }

            const bool is_double_click = item.object && item.object == this->_lastClickedObject
                && this->_clickClock.getElapsedTime().asMilliseconds() < DOUBLE_CLICK_MS;
            this->_lastClickedObject = item.object;
            this->_clickClock.restart();

            if (is_double_click && !shift_pressed
                && (item.type == ItemType::LIGHT || item.type == ItemType::PRIMITIVE || item.type == ItemType::GROUP))
            {
                this->beginRename(item);
                return (true);
            }

            if (item.type == ItemType::CAMERA)
                this->selectCamera();
            else if (shift_pressed)
                this->selectRange(item.object, ctrl_pressed);
            else
                this->select(item.object, ctrl_pressed);
            this->_selectionChanged = true;
            return (true);
        }

        const bool had_selection = this->_cameraSelected || !this->_selection.empty();
        this->_selection.clear();
        this->_cameraSelected = false;
        this->_selectionAnchor = nullptr;
        this->_selectionLead = nullptr;
        this->_dragObject = nullptr;
        if (had_selection)
            this->_selectionChanged = true;
        return (true);
    }

    CursorType HierarchyPanel::getCursor()
    {
        if (!this->enabled)
            return (CursorType::ARROW);
        if (this->_dragActive || this->hovered)
            return (CursorType::HAND);
        return (CursorType::ARROW);
    }

    bool HierarchyPanel::isCapturing() const
    {
        return (this->_renameField.isCapturing() || this->_dragActive);
    }

    void HierarchyPanel::select(const ISceneObject *object, bool ctrlPressed)
    {
        if (!object)
            return;

        this->_cameraSelected = false;
        this->_selectionAnchor = object;
        this->_selectionLead = object;

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
        this->_selectionAnchor = nullptr;
        this->_selectionLead = nullptr;
    }

    void HierarchyPanel::deleteObjects(const std::vector<const ISceneObject *> &objects)
    {
        if (objects.empty() || !this->_onItemDeleteRequest)
            return;

        for (const ISceneObject *object : objects)
        {
            if (!object)
                continue;
            bool coveredByAncestor = false;
            for (const ISceneObject *other : objects)
                if (other && other != object && this->isAncestorOf(other, object))
                {
                    coveredByAncestor = true;
                    break;
                }
            if (!coveredByAncestor)
                this->_onItemDeleteRequest(object);
        }

        this->_selection.clear();
        this->_cameraSelected = false;
        this->_selectionAnchor = nullptr;
        this->_selectionLead = nullptr;
        this->_lastClickedObject = nullptr;
        this->_dragObject = nullptr;
        this->_dragActive = false;
        if (this->_renamingObject)
            this->cancelRename();
        this->_selectionChanged = true;
    }

    void HierarchyPanel::deleteSelection()
    {
        this->deleteObjects(this->_selection);
    }

    void HierarchyPanel::selectRange(const ISceneObject *target, bool additive)
    {
        if (!target)
            return;

        int anchorIdx = -1;
        int targetIdx = -1;
        for (std::size_t i = 0; i < this->_items.size(); ++i)
        {
            const Item &it = this->_items[i];
            if (!it.selectable || !it.object)
                continue;
            if (it.object == this->_selectionAnchor)
                anchorIdx = static_cast<int>(i);
            if (it.object == target)
                targetIdx = static_cast<int>(i);
        }

        if (targetIdx < 0)
            return;
        if (anchorIdx < 0)
        {
            this->select(target, false);
            return;
        }

        if (!additive)
            this->_selection.clear();
        this->_cameraSelected = false;

        const int lo = std::min(anchorIdx, targetIdx);
        const int hi = std::max(anchorIdx, targetIdx);
        for (int i = lo; i <= hi; ++i)
        {
            const Item &it = this->_items[i];
            if (!it.selectable || !it.object)
                continue;
            if (std::find(this->_selection.begin(), this->_selection.end(), it.object) == this->_selection.end())
                this->_selection.push_back(it.object);
        }
        this->_selectionLead = target;
    }

    void HierarchyPanel::moveSelection(int direction, bool ctrlPressed, bool shiftPressed)
    {
        std::vector<std::size_t> selectable;
        for (std::size_t i = 0; i < this->_items.size(); ++i)
            if (this->_items[i].selectable)
                selectable.push_back(i);
        if (selectable.empty())
            return;

        if (shiftPressed)
        {
            int lead = -1;
            for (std::size_t k = 0; k < selectable.size(); ++k)
                if (this->_selectionLead && this->_items[selectable[k]].object == this->_selectionLead)
                {
                    lead = static_cast<int>(k);
                    break;
                }

            int next;
            if (lead < 0)
                next = (direction > 0) ? 0 : static_cast<int>(selectable.size()) - 1;
            else
            {
                next = lead + direction;
                if (next < 0 || next >= static_cast<int>(selectable.size()))
                    return; // Already at the edge of the list.
            }

            const Item &target = this->_items[selectable[next]];
            if (target.type == ItemType::CAMERA)
                return;
            if (!this->_selectionAnchor)
                this->_selectionAnchor = (lead >= 0) ? this->_items[selectable[lead]].object : target.object;
            this->selectRange(target.object, ctrlPressed);
            this->_selectionChanged = true;
            this->_lastClickedObject = target.object;
            return;
        }

        int current = -1;
        for (std::size_t k = 0; k < selectable.size(); ++k)
        {
            if (!this->_items[selectable[k]].selected)
                continue;
            if (direction > 0 || current < 0)
                current = static_cast<int>(k);
        }

        int next;
        if (current < 0)
            next = (direction > 0) ? 0 : static_cast<int>(selectable.size()) - 1;
        else
        {
            next = current + direction;
            if (next < 0 || next >= static_cast<int>(selectable.size()))
                return; // Already at the edge of the list.
        }

        const Item &target = this->_items[selectable[next]];
        if (target.type == ItemType::CAMERA)
            this->selectCamera();
        else
            this->select(target.object, ctrlPressed);
        this->_selectionChanged = true;
        this->_lastClickedObject = target.object;
    }

    void HierarchyPanel::beginRename(const Item &item)
    {
        const ISceneObject *object = item.object;
        if (!object)
            return;

        this->select(object, false);
        this->_selectionChanged = true;

        this->_renamingObject = object;
        const sf::FloatRect rect = renameFieldRect(item.bounds, item.buttonBounds);
        this->_renameField.begin(item.label, rect.left, rect.top, rect.width, rect.height);

        this->_lastClickedObject = nullptr;
    }

    void HierarchyPanel::layoutRenameField(const sf::FloatRect &itemBounds, const sf::FloatRect &buttonBounds)
    {
        const sf::FloatRect rect = renameFieldRect(itemBounds, buttonBounds);
        this->_renameField.relayout(rect.left, rect.top, rect.width, rect.height);
    }

    void HierarchyPanel::commitRename(const std::string &value)
    {
        if (!this->_renamingObject)
            return;

        const size_t first = value.find_first_not_of(" \t");
        const size_t last = value.find_last_not_of(" \t");
        const std::string trimmed = (first == std::string::npos) ? "" : value.substr(first, last - first + 1);

        if (!trimmed.empty())
            const_cast<ISceneObject *>(this->_renamingObject)->setName(trimmed);

        this->_renamingObject = nullptr;
    }

    void HierarchyPanel::cancelRename()
    {
        this->_renamingObject = nullptr;
        this->_renameField.cancel();
    }
}
