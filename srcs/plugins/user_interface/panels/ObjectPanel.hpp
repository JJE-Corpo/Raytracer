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

            std::function<bool(Axis axis, float value)> onVertexEdit;
            std::function<bool(float factor)> onVertexScale;
            void setVertexEditor(bool visible, const Vector3f &value);

            std::function<void(int direction)> onVertexNavigate;
            void setVertexNavigator(bool visible, int index, int count);

            std::function<void()> onConvertToMesh;

            std::function<void(int mode)> onGizmoModeChanged;
            void setGizmoMode(int mode);

            // Gizmo space toggle (World / Local). onGizmoSpaceChanged(local) fires
            // on click; setGizmoSpace reflects the owner's current space.
            std::function<void(bool local)> onGizmoSpaceChanged;
            void setGizmoSpace(bool local);

            void setFont(sf::Font &font) override;
            void layout(float x, float y, float width);
            void setScene(IScene *scene);

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

            // Gizmo tool selector row (Move / Rotate / Scale) at the top, plus a
            // World/Local space toggle just below it.
            Button _gizmoMoveButton;
            Button _gizmoRotateButton;
            Button _gizmoScaleButton;
            Button _gizmoSpaceButton;
            bool _gizmoSpaceLocal = false;

            // Vertex navigator (< Point N / total >), shown above the size row.
            sf::Text _vertexNavLabel;
            Button _vertexPrevButton;
            Button _vertexNextButton;
            bool _showVertexNav = false;

            // One-click uniform grow/shrink of the selected object's scale.
            sf::Text _scaleStepLabel;
            Button _scaleDownButton;
            Button _scaleUpButton;

            // "Convert to Mesh" button, shown for non-editable primitives.
            Button _convertToMeshButton;
            bool _showConvertToMesh = false;

            VectorField _vertexField;
            bool _showVertexEditor = false;

            void applyScaleStep(ISceneObject *object, float factor);

            sf::Font _font;
    };
}

#endif
