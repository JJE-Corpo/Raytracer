//
// Created by jazema on 5/7/26.
//

#ifndef HIERARCHYPANEL_HPP
#define HIERARCHYPANEL_HPP

#include <SFML/Graphics.hpp>
#include <string>
#include <vector>
#include <functional>

#include "../Component.hpp"
#include "../components/TextField.hpp"
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

            void setOnItemHideRequest(std::function<void(const ISceneObject *)> cb)
            {
                this->_onItemHideRequest = cb;
            }

            template <typename ItemType>
            ItemType *tryCast()
            {
                if (this->_selection.size() != 1)
                    return (nullptr);
                return (dynamic_cast<ItemType *>(this->_selection.at(0)));
            }
        private:
            struct Item
            {
                std::string label;
                int depth = 0;
                ItemType type = ItemType::GROUP;
                const void *payload = nullptr;
                bool selectable = false;
                bool selected = false;
                bool hovered = false;
                bool hidden = false;
                sf::FloatRect buttonBounds;
                sf::FloatRect bounds;
            };

            void buildItems();
            void refreshHoverState();
            void select(const ISceneObject *primitive, bool ctrlPressed);
            void selectCamera();

            void beginRename(const Item &item);
            void commitRename();
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
            sf::Vector2i _lastMouse{-100000, -100000};
            std::function<void(const ISceneObject *)> _onItemHideRequest;

            // Inline rename: nullptr when no row is being renamed.
            const ISceneObject *_renamingObject = nullptr;
            TextField _renameField;
            sf::Clock _clickClock;
            const void *_lastClickedPayload = nullptr;
    };
}

#endif
