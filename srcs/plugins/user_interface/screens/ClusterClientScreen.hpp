//
// Created by jazema on 5/16/26.
//

#ifndef CLUSTERCLIENTSCREEN_HPP
#define CLUSTERCLIENTSCREEN_HPP
#include <functional>

#include "Screen.hpp"
#include "../LayoutPen.hpp"
#include "../components/Button.hpp"
#include "../toast/ToastManager.hpp"
#include "../../../common/cluster/IClusterClient.hpp"

namespace rc
{
    struct ClusterClientScreen : Screen
    {
        private:
            void updateRenderPreview()
            {
                sf::Image image;

                if (this->_currentRender.size_x <= 0 || this->_currentRender.size_y <= 0)
                    return;
                image.create(this->_currentRender.size_x, this->_currentRender.size_y);
                for (int i = 0; i < this->_currentRender.size_y; i++)
                {
                    for (int j = 0; j < this->_currentRender.size_x; j++)
                    {
                        size_t index = i * this->_currentRender.size_x + j;
                        if (index >= this->_currentRender.pixels.size())
                            continue;
                        Color current = this->_currentRender.pixels.at(i * this->_currentRender.size_x + j);
                        image.setPixel(j, i, {(current.r), (current.g), (current.b), (current.a)});
                    }
                }
                if (image.getSize() != this->_renderTexture.getSize())
                    this->_renderTexture.loadFromImage(image);
                else
                    this->_renderTexture.update(image);
                this->_renderSprite.setTexture(this->_renderTexture);
            }
        public:
        void setClient(IClusterClient *client)
        {
            this->_clusterClient = client;
            if (!client)
            {
                this->_leaveButton.onClick = {};
                return;
            }
            client->setInfoLogger([this](const std::string &message)
            {
                this->_toastManager.push("Cluster", message, ToastType::INFO);
            });
            client->setErrorLogger([this](const std::string &message)
            {
                this->_toastManager.push("Cluster", message, ToastType::ERROR);
            });
        }

        void setViewportRenderer(ISceneRenderer *viewportRenderer)
        {
            this->_viewportRenderer = viewportRenderer;
        }

        void setOnLeave(std::function<void()> onLeave)
        {
            this->_onLeave = std::move(onLeave);
        }

        ToastManager &getToastManager()
        {
            return (this->_toastManager);
        }

        void setFont(sf::Font &font) override
        {
            this->_title.setFont(font);
            this->_status.setFont(font);
            this->_leaveButton.setFont(font);
            this->_toastManager.setFont(font);
        }

        void handleEvent(sf::RenderWindow &window, sf::Event &event) override
        {
            sf::Vector2i mouse = sf::Mouse::getPosition(window);

            this->_leaveButton.handleEvent(event, mouse);
        }

        protected:

            void update(sf::RenderWindow &window) override
            {
                sf::Vector2i mouse = sf::Mouse::getPosition(window);
                int width = window.getSize().x - 20;
                LayoutPen verticalLayout = {10, 10};
                float viewportScale = 1.f;

                if (this->_clusterClient == nullptr)
                    return;
                if (this->_clusterClient->getServerRenderState() == ServerRenderState::RENDERING)
                {
                    this->_currentRender = Render();
                }
                else
                {
                    IScene *scene = this->_clusterClient->getScene();

                    if (scene)
                    {
                        this->_viewportRenderer->renderScene(*scene);
                        this->_currentRender = this->_viewportRenderer->getRender();
                    }
                    else
                    {
                        this->_currentRender = Render();
                    }
                }
                this->_title.setString("Cluster " + this->_clusterClient->getAddress() + " (Port: " + std::to_string(this->_clusterClient->getPort()) + ")");
                this->_title.setPosition(verticalLayout.x, verticalLayout.y);
                this->_title.setCharacterSize(18);
                this->_title.setFillColor(theme::TEXT_MAIN);
                verticalLayout.next(18);
                this->_status.setString(this->_clusterClient->getStatus());
                this->_status.setPosition(verticalLayout.x, verticalLayout.y);
                this->_status.setCharacterSize(12);
                this->_status.setFillColor(theme::TEXT_WHITE);
                verticalLayout.next(12);
                this->_leaveButton.update(mouse);
                this->_leaveButton.setLabel("Leave cluster");
                this->_leaveButton.onClick = this->_onLeave;
                this->_leaveButton.layout(verticalLayout.x, verticalLayout.y, width, 24);
                verticalLayout.next(24);

                if (this->_currentRender.size_x > 0 && this->_currentRender.size_y > 0)
                {
                    const float scaleX = width / static_cast<float>(this->_currentRender.size_x);
                    const float scaleY = (window.getSize().y - 20) / static_cast<float>(this->_currentRender.size_y);
                    viewportScale = std::min(scaleX, scaleY);
                }

                this->_renderSprite.setScale(viewportScale, viewportScale);
                this->_renderSprite.setPosition({10, 10});
            }

            void draw(sf::RenderWindow &window) override
            {
                if (this->_clusterClient == nullptr)
                    return;
                updateRenderPreview();
                window.draw(this->_renderSprite);
                window.draw(this->_title);
                window.draw(this->_status);
                window.draw(this->_leaveButton);
                this->_toastManager.update();
                this->_toastManager.draw(window);
            }

        private:
            sf::Text _title;
            sf::Text _status;
            sf::Texture _renderTexture;
            sf::Sprite _renderSprite;
            Render _currentRender;

            ToastManager _toastManager;
            Button _leaveButton;
            IClusterClient *_clusterClient = nullptr;
            ISceneRenderer *_viewportRenderer = nullptr;
            std::function<void()> _onLeave = {};
    };
}

#endif
