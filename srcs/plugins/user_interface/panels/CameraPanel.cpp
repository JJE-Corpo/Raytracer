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
        this->_resolutionLabel.setFont(font);
        this->_resolutionLabel.setString("Resolution");
        this->_resolutionLabel.setCharacterSize(12);
        this->_resolutionLabel.setFillColor(theme::TEXT_WHITE);

        this->_widthField.setFont(font);
        this->_widthField.setCharacterSize(12);
        this->_heightField.setFont(font);
        this->_heightField.setCharacterSize(12);

        this->_positionField.setLabel("Position");
        this->_positionField.setFont(font);
        this->_rotationField.setLabel("Rotation");
        this->_rotationField.setFont(font);
    }

    void CameraPanel::rebuild(ICamera *camera)
    {
        this->enabled = true;
        this->_widthField.setValue(std::to_string(camera->getResolution().x));
        this->_widthField.onValidate = [camera](const std::string &value)
        {
            camera->setResolution({std::stoi(value), camera->getResolution().y});
            return (true);
        };
        this->_widthField.onType = [](const std::string &value)
        {
            return (value.empty() || Utils::isUnsignedLong(value));
        };
        this->_heightField.setValue(std::to_string(camera->getResolution().y));
        this->_heightField.onValidate = [camera](const std::string &value)
        {
            camera->setResolution({camera->getResolution().x, std::stoi(value)});
            return (true);
        };
        this->_heightField.onType = [](const std::string &value)
        {
            return (value.empty() || Utils::isUnsignedLong(value));
        };
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

        this->_resolutionLabel.setPosition({layout.x, layout.y});
        this->_widthField.layout(layout.x + 80, layout.y, width - 80, 20);
        layout.next(20);
        this->_heightField.layout(layout.x + 80, layout.y, width - 80, 20);
        layout.next(20);

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
        this->_widthField.update(mouse);
        this->_heightField.update(mouse);
        this->_positionField.update(mouse);
        this->_rotationField.update(mouse);
    }

    bool CameraPanel::isCapturing() const
    {
        return (this->enabled
            && (this->_widthField.isCapturing() || this->_heightField.isCapturing() || this->_positionField.isCapturing() || this->_rotationField.isCapturing()));
    }

    bool CameraPanel::handleEvent(const sf::Event &event, const sf::Vector2i mouse)
    {
        if (!this->enabled)
            return (false);
        bool consumed = this->_widthField.handleEvent(event, mouse);
        consumed = this->_heightField.handleEvent(event, mouse) || consumed;
        consumed = this->_positionField.handleEvent(event, mouse) || consumed;
        consumed = this->_rotationField.handleEvent(event, mouse) || consumed;
        return (consumed);
    }

    void CameraPanel::draw(sf::RenderTarget &target, sf::RenderStates states) const
    {
        target.draw(this->_resolutionLabel, states);
        target.draw(this->_widthField, states);
        target.draw(this->_heightField, states);
        target.draw(this->_positionField, states);
        target.draw(this->_rotationField, states);
    }

    CursorType CameraPanel::getCursor()
    {
        if (!this->enabled)
            return (CursorType::ARROW);

        CursorType result = this->_widthField.getCursor();
        if (result == CursorType::ARROW)
            result = this->_heightField.getCursor();
        if (result == CursorType::ARROW)
            result = this->_positionField.getCursor();
        if (result == CursorType::ARROW)
            result = this->_rotationField.getCursor();
        return (result);
    }
}
