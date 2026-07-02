//
// Created by jazema on 5/17/26.
//

#include "CameraPanel.hpp"

#include "../Theme.hpp"
#include "../LayoutPen.hpp"
#include "../../../common/Axis.hpp"
#include "../../../common/scene/ICamera.hpp"

namespace rc
{
    void CameraPanel::setFont(sf::Font &font)
    {
        this->_positionField.setLabel("Position");
        this->_positionField.setFont(font);
        this->_rotationField.setLabel("Rotation");
        this->_rotationField.setFont(font);

        this->_title.setFont(font);
        this->_title.setString("Camera");
        this->_title.setCharacterSize(14);
        this->_title.setFillColor(theme::TEXT_DIM);
    }

    void CameraPanel::rebuild(ICamera *camera)
    {
        this->enabled = true;
        this->_positionField.setValue(camera->getPosition());
        this->_positionField.onValidate = [camera](Axis axis, float value)
        {
            Vector3f result = camera->getPosition();
            if (axis == Axis::X) result.x = value;
            if (axis == Axis::Y) result.y = value;
            if (axis == Axis::Z) result.z = value;
            camera->setPosition(result);
            return (true);
        };
        this->_rotationField.setValue(camera->getRotation());
        this->_rotationField.onValidate = [camera](Axis axis, float value)
        {
            Vector3f result = camera->getRotation();
            if (axis == Axis::X) result.x = value;
            if (axis == Axis::Y) result.y = value;
            if (axis == Axis::Z) result.z = value;
            camera->setRotation(result);
            return (true);
        };
    }

    void CameraPanel::layout(float x, float y, float width)
    {
        LayoutPen layout{x, y};

        this->_title.setPosition({layout.x, layout.y});
        layout.next(24);

        this->_positionField.layout(layout.x, layout.y, width);
        layout.next(32);
        this->_rotationField.layout(layout.x, layout.y, width);
        layout.next(32);

        this->height = layout.y - y;
    }

    void CameraPanel::unselect()
    {
        this->enabled = false;
    }

    void CameraPanel::update(sf::Vector2i mouse)
    {
        if (!this->enabled)
            return;
        this->_positionField.update(mouse);
        this->_rotationField.update(mouse);
    }

    bool CameraPanel::isCapturing() const
    {
        return (this->enabled
            && (this->_positionField.isCapturing() || this->_rotationField.isCapturing()));
    }

    bool CameraPanel::handleEvent(const sf::Event &event, const sf::Vector2i mouse)
    {
        if (!this->enabled)
            return (false);
        bool consumed = this->_positionField.handleEvent(event, mouse);
        consumed = this->_rotationField.handleEvent(event, mouse) || consumed;
        return (consumed);
    }

    void CameraPanel::draw(sf::RenderTarget &target, sf::RenderStates states) const
    {
        target.draw(this->_title, states);
        target.draw(this->_positionField, states);
        target.draw(this->_rotationField, states);
    }
}
