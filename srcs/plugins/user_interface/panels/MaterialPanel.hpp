//
// Created by jazema on 5/11/26.
//

#ifndef MATERIALPANEL_HPP
#define MATERIALPANEL_HPP

#include <vector>

#include "../Component.hpp"
#include "../components/ColorPicker.hpp"
#include "../components/InlineEditField.hpp"
#include "../components/SegmentedControl.hpp"
#include "../components/Slider.hpp"

namespace rc
{
    class ISceneObject;
    struct Material;

    class MaterialPanel : public Component
    {
        public:
            float height = 0;

            void setFont(sf::Font &font) override;
            void layout(float x, float y, float width);

            void rebuild(const ISceneObject *currentObject);

            CursorType getCursor() override;
            bool isCapturing() const override;
            void update(sf::Vector2i mouse) override;
            bool handleEvent(const sf::Event &event, sf::Vector2i mouse) override;
            void draw(sf::RenderTarget &target, sf::RenderStates states) const override;
            void drawOverlay(sf::RenderTarget &target, sf::RenderStates states) const override;

        private:
            void beginNameEdit();
            void commitName(const std::string &value);

            void persistMaterial();

            sf::Text _materialName;
            SegmentedControl _materialModelSelector;
            ColorPicker _baseColorPicker;
            std::vector<Slider> _materialSliders;

            Material *_material = nullptr;

            InlineEditField _nameField;
            sf::FloatRect _nameRect;
            bool _nameHovered = false;
            bool _namePendingClick = false;
            sf::Clock _nameClickClock;

            sf::Font _font;
    };
}

#endif
