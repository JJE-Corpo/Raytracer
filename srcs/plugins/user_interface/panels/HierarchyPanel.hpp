//
// Created by jazema on 5/7/26.
//

#ifndef HIERARCHYPANEL_HPP
#define HIERARCHYPANEL_HPP

#include <SFML/Graphics.hpp>
#include <string>
#include <vector>
#include <set>
#include <functional>

#include "../Component.hpp"
#include "../components/InlineEditField.hpp"
#include "../../../common/scene/ISceneObject.hpp"

namespace rc
{
    class IScene;
    class IPrimitive;
    class ILight;

    class HierarchyPanel : public Component
    {
        public:
            enum class ItemType
            {
                CAMERA,
                LIGHT,
                PRIMITIVE,
                GROUP
            };

            void setFont(sf::Font &font) override;
            void setScene(IScene *scene);
            void layout(float x, float y, float width);
            float getBottomY() const;

            void draw(sf::RenderTarget &target, sf::RenderStates states) const override;
            void update(sf::Vector2i mouse) override;
            bool handleEvent(const sf::Event &event, sf::Vector2i mouse) override;
            CursorType getCursor() override;
            bool isCapturing() const override;

            // Total laid-out content height (the wrapping ScrollView reads this).
            float height = 0.f;

            const std::vector<const ISceneObject *> &getSelection() const;

            bool isCameraSelected() const;

            bool isSelected(const ISceneObject *object) const;

            void applyViewportSelection(const std::vector<const ISceneObject *> &selection);
            bool consumeSelectionChanged();

            // A right-click on a row records the object here; the screen polls
            // this after routing an event (like consumeSelectionChanged) and
            // opens the context menu at the cursor. Returns nullptr when no
            // request is pending, clearing it in the process.
            const ISceneObject *consumeContextMenuRequest();

            void setOnItemHideRequest(std::function<void(const ISceneObject *)> cb)
            {
                this->_onItemHideRequest = cb;
            }

            // Fired once per object the user asks to delete (row button or the
            // Delete key). The scene owns the actual removal; the panel only
            // signals intent and then drops the freed objects from its state.
            void setOnItemDeleteRequest(std::function<void(const ISceneObject *)> cb)
            {
                this->_onItemDeleteRequest = cb;
            }

            // Delete the current object selection (the camera row is never part
            // of it, so it is safe from here). Used by the Delete/Suppr key,
            // including the screen-level fallback when the cursor is outside the
            // panel and the key never reaches handleEvent().
            void deleteSelection();

            // Fired when the user drops a row onto a group (nest), between rows
            // (reorder) or past the list (move to root):
            // (child, newParent, index) with newParent == nullptr for the top
            // level and index the destination slot among the new siblings
            // (counted without child); index < 0 means append.
            void setOnReparentRequest(std::function<void(const ISceneObject *, const ISceneObject *, int)> cb)
            {
                this->_onReparentRequest = cb;
            }

            template <typename ItemType>
            ItemType *tryCast()
            {
                if (this->_selection.size() != 1)
                    return (nullptr);
                return (dynamic_cast<ItemType *>(this->_selection.at(0)));
            }
        private:
            // Where a dragged row would land relative to the row under the cursor.
            enum class DropMode
            {
                NONE,
                INTO,   // become a child of the target group (appended)
                SIBLING // slot into a gap between rows (reorder / move to root)
            };

            struct Item
            {
                std::string label;
                int depth = 0;
                ItemType type = ItemType::GROUP;
                // Properly up-cast scene object for this row (nullptr for camera).
                // Never round-trip identity through void*: with ISceneObject a
                // virtual base, an IPrimitive*/ILight* differs from its
                // ISceneObject* by an offset, so a void* reinterpret would break
                // selection/hover matching against hit.primitive in the viewport.
                const ISceneObject *object = nullptr;
                bool selectable = false;
                bool selected = false;
                bool hovered = false;
                bool hidden = false;
                bool expandable = false;
                bool expanded = true;
                sf::FloatRect buttonBounds;
                sf::FloatRect deleteButtonBounds;
                sf::FloatRect toggleBounds;
                sf::FloatRect bounds;
            };

            void buildItems();
            void refreshHoverState();
            void select(const ISceneObject *primitive, bool ctrlPressed);
            // Select the contiguous run of object rows between the selection
            // anchor and target (inclusive), in displayed order. additive merges
            // the run into the current selection (Ctrl+Shift) rather than
            // replacing it.
            void selectRange(const ISceneObject *target, bool additive);
            void selectCamera();

            bool isExpanded(const ISceneObject *group) const;
            // True if ancestor is node itself or one of its (transitive) parents.
            bool isAncestorOf(const ISceneObject *ancestor, const ISceneObject *node) const;
            void updateDropTarget(sf::Vector2i mouse);
            // Configure a between-rows (SIBLING) drop plus its indicator line.
            void setSiblingDrop(const ISceneObject *parent, int index, float lineX, float lineY, float lineW);
            // Slot `ref` occupies among `parent`'s children (roots when parent is
            // null), skipping the dragged node (it is detached before the insert),
            // i.e. the index to land the dragged node just before `ref`.
            int siblingIndexOf(const ISceneObject *parent, const ISceneObject *ref) const;
            void commitDrop();

            // Move the selection to the previous (-1) or next (+1) selectable
            // row in response to an Up/Down arrow key. With ctrlPressed the newly
            // reached row is added to the current selection instead of replacing
            // it, matching Ctrl+click multi-select. With shiftPressed the range
            // from the anchor to the newly reached row is selected, matching
            // Shift+click.
            void moveSelection(int direction, bool ctrlPressed, bool shiftPressed);

            // Fire the delete callback for `objects`, then wipe every transient
            // pointer into the scene (selection, anchors, drag, rename): a group
            // delete frees its subtree, so nested references may now dangle.
            // Objects covered by an ancestor already in the batch are skipped so
            // the callback never runs on freed memory.
            void deleteObjects(const std::vector<const ISceneObject *> &objects);

            void beginRename(const Item &item);
            void commitRename(const std::string &value);
            void cancelRename();
            void layoutRenameField(const sf::FloatRect &itemBounds, const sf::FloatRect &buttonBounds);

            sf::Font *_font = nullptr;
            IScene *_scene = nullptr;
            std::vector<Item> _items;
            float _originX = 0.f;
            float _originY = 0.f;
            float _width = 0.f;
            float _bottomY = 0.f;

            std::vector<const ISceneObject *> _selection;
            bool _cameraSelected = false;
            bool _selectionChanged = false;
            // Object the user right-clicked, awaiting the screen to open the
            // context menu for it (see consumeContextMenuRequest).
            const ISceneObject *_contextMenuRequest = nullptr;
            // Range-selection pivot: _selectionAnchor is the fixed end of a
            // Shift range; _selectionLead is its moving end (the last row a
            // click or arrow landed on). Both null when nothing anchors a range.
            const ISceneObject *_selectionAnchor = nullptr;
            const ISceneObject *_selectionLead = nullptr;
            sf::Vector2i _lastMouse{-100000, -100000};
            std::function<void(const ISceneObject *)> _onItemHideRequest;
            std::function<void(const ISceneObject *)> _onItemDeleteRequest;
            std::function<void(const ISceneObject *, const ISceneObject *, int)> _onReparentRequest;

            // Groups collapsed by the user (default is expanded).
            std::set<const ISceneObject *> _collapsed;

            // Drag-and-drop reparenting state.
            const ISceneObject *_dragObject = nullptr;
            bool _dragActive = false;
            sf::Vector2i _dragStartMouse{0, 0};
            int _dropRow = -1;
            const ISceneObject *_dropParent = nullptr;
            DropMode _dropMode = DropMode::NONE;
            // Destination slot for a SIBLING drop (index among _dropParent's
            // children, or the roots); < 0 means append. Ignored for INTO.
            int _dropIndex = -1;
            // Geometry of the SIBLING insertion indicator line.
            float _dropLineX = 0.f;
            float _dropLineY = 0.f;
            float _dropLineW = 0.f;

            // Inline rename: nullptr when no row is being renamed.
            const ISceneObject *_renamingObject = nullptr;
            InlineEditField _renameField;
            sf::Clock _clickClock;
            const ISceneObject *_lastClickedObject = nullptr;
    };
}

#endif
