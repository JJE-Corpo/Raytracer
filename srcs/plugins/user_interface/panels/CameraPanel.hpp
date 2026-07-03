//
// Created by jazema on 5/17/26.
//

#ifndef CAMERAPANEL_HPP
#define CAMERAPANEL_HPP

#include "../Component.hpp"
#include "../components/VectorField.hpp"

namespace rc
{
    class ICamera;

    class CameraPanel : public Component
    {
        public:
            float height = 0.0;

            void setFont(sf::Font &font) override;

            // Loads the camera's position/rotation into the fields and wires their
            // onValidate callbacks to write straight back into it.
            void rebuild(ICamera *camera);

            void layout(float x, float y, float width);
            void unselect();

            void update(sf::Vector2i mouse) override;
            bool isCapturing() const override;
            bool handleEvent(const sf::Event &event, sf::Vector2i mouse) override;
            void draw(sf::RenderTarget &target, sf::RenderStates states) const override;

        private:
            sf::Text _title;

            VectorField _positionField;
            VectorField _rotationField;
    };
}

#endif
