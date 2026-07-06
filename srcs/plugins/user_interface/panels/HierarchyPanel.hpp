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

            float height = 0.f;

            const std::vector<const ISceneObject *> &getSelection() const;

            bool isCameraSelected() const;

            bool isSelected(const ISceneObject *object) const;

            void applyViewportSelection(const std::vector<const ISceneObject *> &selection);
            bool consumeSelectionChanged();

            const ISceneObject *consumeContextMenuRequest();

            void setOnItemHideRequest(std::function<void(const ISceneObject *)> cb)
            {
                this->_onItemHideRequest = cb;
            }

            void setOnItemDeleteRequest(std::function<void(const ISceneObject *)> cb)
            {
                this->_onItemDeleteRequest = cb;
            }

            void deleteSelection();

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
            void selectRange(const ISceneObject *target, bool additive);
            void selectCamera();

            bool isExpanded(const ISceneObject *group) const;
            bool isAncestorOf(const ISceneObject *ancestor, const ISceneObject *node) const;
            void updateDropTarget(sf::Vector2i mouse);
            void setSiblingDrop(const ISceneObject *parent, int index, float lineX, float lineY, float lineW);
            int siblingIndexOf(const ISceneObject *parent, const ISceneObject *ref) const;
            void commitDrop();

            void moveSelection(int direction, bool ctrlPressed, bool shiftPressed);

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
            const ISceneObject *_contextMenuRequest = nullptr;
            const ISceneObject *_selectionAnchor = nullptr;
            const ISceneObject *_selectionLead = nullptr;
            sf::Vector2i _lastMouse{-100000, -100000};
            std::function<void(const ISceneObject *)> _onItemHideRequest;
            std::function<void(const ISceneObject *)> _onItemDeleteRequest;
            std::function<void(const ISceneObject *, const ISceneObject *, int)> _onReparentRequest;

            std::set<const ISceneObject *> _collapsed;

            // Drag-and-drop reparenting state.
            const ISceneObject *_dragObject = nullptr;
            bool _dragActive = false;
            sf::Vector2i _dragStartMouse{0, 0};
            int _dropRow = -1;
            const ISceneObject *_dropParent = nullptr;
            DropMode _dropMode = DropMode::NONE;
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
