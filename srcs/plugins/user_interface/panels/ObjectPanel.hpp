//
// Created by jazema on 5/11/26.
//

#ifndef OBJECTPANEL_HPP
#define OBJECTPANEL_HPP

#include <functional>
#include <vector>

#include "../Component.hpp"
#include "../components/Button.hpp"
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

            // Vertex edit mode: when shown, a coordinate field for the vertex
            // currently selected in the viewport. onVertexEdit applies a
            // keyboard edit (world coordinate) back onto that vertex; the screen
            // keeps the displayed value in sync with the live drag.
            std::function<bool(Axis axis, float value)> onVertexEdit;
            // When a vertex is selected (the vertex editor is visible), the -/+
            // buttons grow/shrink THAT vertex relative to the shape centre
            // instead of scaling the whole object. Returns true if handled.
            std::function<bool(float factor)> onVertexScale;
            void setVertexEditor(bool visible, const Vector3f &value);

            // Small "< Point N / total >" navigator shown above the -/+ buttons
            // when the selected object is editable, to step through its vertices
            // without aiming at the on-screen handles. onVertexNavigate(dir) is
            // called with -1 / +1; the owner updates the label via this setter.
            std::function<void(int direction)> onVertexNavigate;
            void setVertexNavigator(bool visible, int index, int count);

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

            // Vertex navigator (< Point N / total >), shown above the size row.
            sf::Text _vertexNavLabel;
            Button _vertexPrevButton;
            Button _vertexNextButton;
            bool _showVertexNav = false;

            // One-click uniform grow/shrink of the selected object's scale.
            sf::Text _scaleStepLabel;
            Button _scaleDownButton;
            Button _scaleUpButton;

            VectorField _vertexField;
            bool _showVertexEditor = false;

            // Multiplies the object's local scale uniformly (used by the -/+ buttons).
            void applyScaleStep(ISceneObject *object, float factor);

            sf::Font _font;
    };
}

#endif
