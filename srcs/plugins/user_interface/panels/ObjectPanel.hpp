//
// Created by jazema on 5/11/26.
//

#ifndef OBJECTPANEL_HPP
#define OBJECTPANEL_HPP

#include <functional>
#include <vector>

#include "../Component.hpp"
#include "../components/ColorPicker.hpp"
#include "../components/Dropdown.hpp"
#include "../components/VectorField.hpp"

namespace rc
{
    class IScene;
    class ISceneObject;
    struct Material;

    class ObjectPanel : public Component
    {
        public:
            float height = 0.0;

            bool isLight = false;
            bool isPrimitive = false;
            std::function<void()> onSceneMutated;

            void setFont(sf::Font &font) override;
            void layout(float x, float y, float width);
            void setScene(IScene *scene);

            // Rebuilds the light/primitive-specific fields (color, intensity,
            // material dropdown, property sliders) for currentObject.
            void rebuild(const ISceneObject *currentObject);

            void update(sf::Vector2i mouse) override;
            bool isCapturing() const override;
            bool handleEvent(const sf::Event &event, sf::Vector2i mouse) override;
            void draw(sf::RenderTarget &target, sf::RenderStates states) const override;
            void drawOverlay(sf::RenderTarget &target, sf::RenderStates states) const override;
            CursorType getCursor() override;

            bool consumeMaterialChanged();

        private:
            // sf::Text _title;

            IScene *scene = nullptr;

            sf::Text _materialLabel;
            Dropdown _materialDropdown;
            std::vector<const Material *> _materials;

            bool _materialChanged = false;

            ColorPicker _lightColorPicker;
            sf::Text _intensityLabel;
            TextField _lightIntensityField;
            std::vector<Slider> _objectSliders;

            VectorField _positionField;
            VectorField _rotationField;
            VectorField _scaleField;

            sf::Font _font;
    };
}

#endif
