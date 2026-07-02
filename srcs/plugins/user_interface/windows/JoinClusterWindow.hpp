//
// Created by jazema on 5/2/26.
//

#ifndef CLUSTERWINDOW_HPP
#define CLUSTERWINDOW_HPP

#include "Window.hpp"
#include "../../../common/Utils.hpp"
#include "../components/Button.hpp"
#include "../components/TextField.hpp"

namespace rc
{
    struct JoinClusterWindow : Window
    {
        std::function<void(std::string,size_t)> windowCallback = {};

        void create(const sf::Font &font)
        {
            this->_font = font;
            this->running = true;
            this->_windowStyle = sf::Style::Close;
            this->windowWidth = 400;
            this->windowHeight = 180;
            this->windowTitle = "Join cluster";
            this->thread = std::thread(&JoinClusterWindow::loop, this);
        }

        void updateUi() override
        {
            sf::Vector2i mouse = sf::Mouse::getPosition(this->window);
            LayoutPen vertical_layout{10, 10};
            float width = static_cast<float>(this->window.getSize().x) - 20.0f;

            this->_title.setFont(this->_font);
            this->_title.setFillColor(theme::TEXT_WHITE);
            this->_title.setCharacterSize(18);
            this->_title.setString("Join a cluster");
            this->_title.setPosition({vertical_layout.x, vertical_layout.y});
            vertical_layout.next(22);
            this->_ipLabel.setFont(this->_font);
            this->_ipLabel.setFillColor(theme::TEXT_DIM);
            this->_ipLabel.setCharacterSize(12);
            this->_ipLabel.setString("IP:");
            this->_ipLabel.setPosition(vertical_layout.x, vertical_layout.y);
            vertical_layout.next(8);
            this->_ipField.setFont(this->_font);
            this->_ipField.layout(vertical_layout.x, vertical_layout.y, width, 24);
            this->_ipField.update(mouse);
            this->_ipField.onValidate = [this](const std::string&)
            {
                this->_portField.focused = true;
                return (true);
            };
            vertical_layout.next(22);
            this->_portLabel.setFont(this->_font);
            this->_portLabel.setFillColor(theme::TEXT_DIM);
            this->_portLabel.setCharacterSize(12);
            this->_portLabel.setString("Port:");
            this->_portLabel.setPosition(vertical_layout.x, vertical_layout.y);
            vertical_layout.next(8);
            this->_portField.setFont(this->_font);
            this->_portField.layout(vertical_layout.x, vertical_layout.y, width, 24);
            this->_portField.update(mouse);
            this->_portField.onType = [](const std::string &value)
            {
                if (!Utils::isUnsignedLong(value))
                    return (false);
                return (true);
            };
            this->_portField.onValidate = [this](const std::string &value)
            {
                if (value.empty())
                    return (false);
                this->windowCallback(this->_ipField.value, std::stoul(this->_portField.value));
                this->destroy();
                return (true);
            };
            vertical_layout.next(26);
            this->_joinButton.setFont(this->_font);
            this->_joinButton.setLabel("JOIN");
            this->_joinButton.layout(vertical_layout.x, vertical_layout.y, width, 24);
            this->_joinButton.update(mouse);
            this->_joinButton.onClick = [this]
            {
                if (this->_portField.value.empty())
                    return ;
                this->windowCallback(this->_ipField.value, std::stoul(this->_portField.value));
                this->destroy();
            };
        }

        void handleEvent(const sf::Event &event) override
        {
            sf::Vector2i mouse = sf::Mouse::getPosition(this->window);

            this->_portField.handleEvent(event, mouse); //lordre est tres important malheureusement dans le system actuel si on met port apres ca marche plus
            this->_ipField.handleEvent(event, mouse);
            this->_joinButton.handleEvent(event, mouse);
        }

        void drawUi() override
        {
            this->window.draw(this->_title);
            this->window.draw(this->_ipLabel);
            this->window.draw(this->_ipField);
            this->window.draw(this->_portLabel);
            this->window.draw(this->_portField);
            this->window.draw(this->_joinButton);
        }

        private:
            sf::Text _title;
            sf::Font _font;
            sf::Text _ipLabel;
            sf::Text _portLabel;
            TextField _ipField;
            TextField _portField;
            Button _joinButton;
    };
}

#endif
