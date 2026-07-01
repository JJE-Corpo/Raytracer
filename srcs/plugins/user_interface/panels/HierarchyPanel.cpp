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
    constexpr float HEADER_HEIGHT = 16.f;
    constexpr float ITEM_SPACING = 6.f;
    constexpr int MAX_VISIBLE_ITEMS = 8;
    constexpr float SCROLLBAR_WIDTH = 8.f;
    constexpr float SCROLLBAR_GAP = 6.f;
    constexpr float SCROLLBAR_MIN_THUMB = 14.f;
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

        auto clampScrollOffset = [&]()
        {
            int maxOffset = std::max(0, this->_totalItems - this->_visibleItems);
            this->_scrollOffset = std::clamp(this->_scrollOffset, 0, maxOffset);
        };

        auto updatePanelBounds = [&]()
        {
            float height = HEADER_HEIGHT + ITEM_SPACING;
            if (this->_visibleItems > 0)
                height += static_cast<float>(this->_visibleItems) * (ITEM_HEIGHT + ITEM_SPACING);
            this->_panelBounds = {this->_originX, this->_originY, this->_width, height};
            this->_bottomY = this->_originY + height;
        };

        auto updateScrollbarGeometry = [&]()
        {
            if (!this->_scrollbarVisible)
            {
                this->_scrollbarTrack = {};
                this->_scrollbarThumb = {};
                return;
            }

            float listHeight = static_cast<float>(this->_visibleItems) * (ITEM_HEIGHT + ITEM_SPACING);
            float trackHeight = std::max(0.f, listHeight - ITEM_SPACING);
            float trackX = this->_originX + this->_contentWidth + SCROLLBAR_GAP;
            float trackY = this->_originY + HEADER_HEIGHT + ITEM_SPACING;
            this->_scrollbarTrack = {trackX, trackY, SCROLLBAR_WIDTH, trackHeight};

            float thumbHeight = trackHeight;
            if (this->_totalItems > 0)
                thumbHeight = std::max(SCROLLBAR_MIN_THUMB, trackHeight * (static_cast<float>(this->_visibleItems) / static_cast<float>(this->_totalItems)));
            thumbHeight = std::min(thumbHeight, trackHeight);

            float thumbTop = trackY;
            int maxOffset = std::max(0, this->_totalItems - this->_visibleItems);
            if (maxOffset > 0 && trackHeight > thumbHeight)
            {
                float t = static_cast<float>(this->_scrollOffset) / static_cast<float>(maxOffset);
                t = std::clamp(t, 0.f, 1.f);
                thumbTop = trackY + (trackHeight - thumbHeight) * t;
            }
            this->_scrollbarThumb = {trackX, thumbTop, SCROLLBAR_WIDTH, thumbHeight};
        };

        float y = this->_originY + HEADER_HEIGHT + ITEM_SPACING;

        if (!this->_scene)
        {
            this->_totalItems = 1;
            this->_visibleItems = std::min(this->_totalItems, MAX_VISIBLE_ITEMS);
            this->_scrollbarVisible = false;
            this->_scrollbarDragging = false;
            this->_contentWidth = this->_width;
            clampScrollOffset();
            Item empty;
            empty.label = "No scene loaded";
            empty.depth = 0;
            empty.type = ItemType::GROUP;
            empty.selectable = false;
            empty.selected = false;
            empty.hovered = false;
            empty.bounds = {this->_originX, y, this->_width, ITEM_HEIGHT};
            this->_items.push_back(empty);
            updatePanelBounds();
            updateScrollbarGeometry();
            return;
        }

        this->_totalItems = 1 + static_cast<int>(this->_scene->getLights().size()) + static_cast<int>(this->_scene->getPrimitives().size());
        this->_visibleItems = std::min(this->_totalItems, MAX_VISIBLE_ITEMS);
        this->_scrollbarVisible = this->_totalItems > this->_visibleItems;
        if (!this->_scrollbarVisible)
            this->_scrollbarDragging = false;
        this->_contentWidth = this->_width - (this->_scrollbarVisible ? (SCROLLBAR_WIDTH + SCROLLBAR_GAP) : 0.f);
        this->_contentWidth = std::max(0.f, this->_contentWidth);
        clampScrollOffset();

        int itemIndex = 0;
        int firstVisible = this->_scrollOffset;
        int lastVisible = this->_scrollOffset + this->_visibleItems - 1;
        auto shouldDrawIndex = [&](int index)
        {
            return index >= firstVisible && index <= lastVisible;
        };

        auto push_item = [&](const std::string &label, int depth, ItemType type, const void *payload, bool selectable, bool selected)
        {
            const int currentIndex = itemIndex++;
            if (!shouldDrawIndex(currentIndex))
                return;
            const float indent = ITEM_INDENT * static_cast<float>(depth);
            float itemX = this->_originX + indent;
            float itemW = std::max(0.f, this->_contentWidth - indent);

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

        updatePanelBounds();
        updateScrollbarGeometry();
    }

    void HierarchyPanel::draw(sf::RenderTarget &target, sf::RenderStates states) const
    {
        if (!this->_font)
            return;

        sf::Text header;
        header.setFont(*this->_font);
        header.setString("Hierarchy");
        header.setCharacterSize(12);
        header.setFillColor(theme::TEXT_DIM);
        header.setPosition({this->_originX, this->_originY});
        target.draw(header, states);

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

        if (this->_scrollbarVisible)
        {
            sf::RectangleShape track;
            track.setPosition({this->_scrollbarTrack.left, this->_scrollbarTrack.top});
            track.setSize({this->_scrollbarTrack.width, this->_scrollbarTrack.height});
            track.setFillColor(theme::BG_CONTROL);
            target.draw(track, states);

            sf::RectangleShape thumb;
            thumb.setPosition({this->_scrollbarThumb.left, this->_scrollbarThumb.top});
            thumb.setSize({this->_scrollbarThumb.width, this->_scrollbarThumb.height});
            thumb.setFillColor(this->_scrollbarDragging || this->_scrollbarHovered ? theme::TEXT_WHITE : theme::TEXT_DIM);
            target.draw(thumb, states);
        }
    }

    void HierarchyPanel::update(sf::Vector2i mouse)
    {
        this->hovered = false;
        this->_scrollbarHovered = false;
        if (this->_scrollbarVisible)
        {
            this->_scrollbarHovered = this->_scrollbarThumb.contains(static_cast<sf::Vector2f>(mouse));
            if (this->_scrollbarDragging && this->_scrollbarTrack.height > 0.f && this->_totalItems > this->_visibleItems)
            {
                float thumbTop = static_cast<float>(mouse.y) - this->_scrollbarDragOffset;
                float minY = this->_scrollbarTrack.top;
                float maxY = this->_scrollbarTrack.top + this->_scrollbarTrack.height - this->_scrollbarThumb.height;
                thumbTop = std::clamp(thumbTop, minY, maxY);
                float t = 0.f;
                if (maxY > minY)
                    t = (thumbTop - minY) / (maxY - minY);
                int maxOffset = std::max(0, this->_totalItems - this->_visibleItems);
                int newOffset = static_cast<int>(std::round(t * static_cast<float>(maxOffset)));
                if (newOffset != this->_scrollOffset)
                {
                    this->_scrollOffset = newOffset;
                    this->buildItems();
                }
            }
        }
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
        if (event.type == sf::Event::MouseButtonReleased && event.mouseButton.button == sf::Mouse::Left)
        {
            const bool wasDragging = this->_scrollbarDragging;
            this->_scrollbarDragging = false;
            return (wasDragging);
        }
        if (event.type == sf::Event::MouseWheelScrolled)
        {
            if (!this->_panelBounds.contains(static_cast<sf::Vector2f>(mouse)))
                return (false);
            if (this->_totalItems <= this->_visibleItems)
                return (false);
            int step = 0;
            if (event.mouseWheelScroll.delta > 0.f)
                step = -1;
            else if (event.mouseWheelScroll.delta < 0.f)
                step = 1;
            if (step != 0)
            {
                this->_scrollOffset += step;
                int maxOffset = std::max(0, this->_totalItems - this->_visibleItems);
                this->_scrollOffset = std::clamp(this->_scrollOffset, 0, maxOffset);
                this->buildItems();
            }
            return (true);
        }
        if (event.type != sf::Event::MouseButtonPressed)
            return (false);
        if (event.mouseButton.button != sf::Mouse::Left)
            return (false);

        if (this->_scrollbarVisible && this->_scrollbarThumb.contains(static_cast<sf::Vector2f>(mouse)))
        {
            this->_scrollbarDragging = true;
            this->_scrollbarDragOffset = static_cast<float>(mouse.y) - this->_scrollbarThumb.top;
            return (true);
        }

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
        if (this->_scrollbarDragging || this->_scrollbarHovered)
            return (CursorType::HAND);
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
