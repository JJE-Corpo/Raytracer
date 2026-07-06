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

            // Rebuilds the model selector, base-color picker and property sliders
            // from currentObject's material (or clears them if it has none).
            void rebuild(const ISceneObject *currentObject);

            CursorType getCursor() override;
            bool isCapturing() const override;
            void update(sf::Vector2i mouse) override;
            bool handleEvent(const sf::Event &event, sf::Vector2i mouse) override;
            void draw(sf::RenderTarget &target, sf::RenderStates states) const override;
            void drawOverlay(sf::RenderTarget &target, sf::RenderStates states) const override;

        private:
            // Opens the inline editor over the name label and commits the typed
            // text back to the material.
            void beginNameEdit();
            void commitName(const std::string &value);

            // Mirror the currently shown material back to the materials market
            // (~/.raytracer/materials) so UI edits persist there. No-op when no
            // material is shown.
            void persistMaterial();

            sf::Text _materialName;
            SegmentedControl _materialModelSelector;
            ColorPicker _baseColorPicker;
            std::vector<Slider> _materialSliders;

            // The material behind _materialName, kept so an inline rename can
            // write straight back to it. Null when no material is shown.
            Material *_material = nullptr;

            // Double-click the name label to rename it, mirroring the hierarchy
            // panel's object rename and a slider value's inline edit.
            InlineEditField _nameField;
            sf::FloatRect _nameRect;
            bool _nameHovered = false;
            bool _namePendingClick = false;
            sf::Clock _nameClickClock;

            sf::Font _font;
    };
}

#endif
