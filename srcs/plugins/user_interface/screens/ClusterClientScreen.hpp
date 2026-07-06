//
// Created by jazema on 5/16/26.
//

#ifndef CLUSTERCLIENTSCREEN_HPP
#define CLUSTERCLIENTSCREEN_HPP
#include <functional>

#include "AScreen.hpp"
#include "../LayoutPen.hpp"
#include "../components/Button.hpp"
#include "../toast/ToastManager.hpp"
#include "../../../common/cluster/IClusterClient.hpp"

namespace rc
{
    struct ClusterClientScreen : AScreen
    {
        private:
            static constexpr float MARGIN = 12.f;
            static constexpr float CARD_W = 300.f;

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
                this->_renderSprite.setTexture(this->_renderTexture, true);
            }

            static const char *activityLabel(IClusterClient::ClientState state)
            {
                switch (state)
                {
                    case IClusterClient::ClientState::FETCHING_DATA:  return ("Fetching scene");
                    case IClusterClient::ClientState::RECEIVING_DATA: return ("Receiving scene");
                    case IClusterClient::ClientState::RENDERING:      return ("Rendering tiles");
                    case IClusterClient::ClientState::SENDING_DATA:   return ("Sending tiles");
                    default:                                          return ("Idle");
                }
            }

            static const char *connectionLabel(ConnectionState state)
            {
                switch (state)
                {
                    case ConnectionState::CONNECTED:    return ("Connected");
                    case ConnectionState::PENDING:      return ("Connecting...");
                    case ConnectionState::REFUSED:      return ("Refused");
                    default:                            return ("Disconnected");
                }
            }

            static sf::Color connectionColor(ConnectionState state)
            {
                switch (state)
                {
                    case ConnectionState::CONNECTED:    return (theme::CHECKED);
                    case ConnectionState::PENDING:      return (theme::ACCENT);
                    default:                            return (theme::TOAST_ERROR);
                }
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
            this->_line.setFont(font);
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

                if (this->_clusterClient == nullptr)
                    return;
                if (this->_clusterClient->getServerRenderState() == ServerRenderState::RENDERING)
                {
                    this->_currentRender = Render();
                }
                else
                {
                    IScene *scene = this->_clusterClient->getScene();

                    if (scene && this->_viewportRenderer)
                    {
                        this->_viewportRenderer->renderScene(*scene);
                        this->_currentRender = this->_viewportRenderer->getRender();
                    }
                    else
                    {
                        this->_currentRender = Render();
                    }
                }

                // Leave button sits at the bottom of the info card.
                const float cardH = this->cardHeight();
                this->_leaveButton.update(mouse);
                this->_leaveButton.setLabel("Leave cluster");
                this->_leaveButton.onClick = this->_onLeave;
                this->_leaveButton.layout(MARGIN + 12.f, MARGIN + cardH - 34.f, CARD_W - 24.f, 24.f);

                // Preview fills the area to the right of the card.
                const float previewX = MARGIN + CARD_W + MARGIN;
                const float availW = static_cast<float>(window.getSize().x) - previewX - MARGIN;
                const float availH = static_cast<float>(window.getSize().y) - 2 * MARGIN;
                float viewportScale = 1.f;

                if (this->_currentRender.size_x > 0 && this->_currentRender.size_y > 0 && availW > 0 && availH > 0)
                {
                    const float scaleX = availW / static_cast<float>(this->_currentRender.size_x);
                    const float scaleY = availH / static_cast<float>(this->_currentRender.size_y);
                    viewportScale = std::min(scaleX, scaleY);
                }
                this->_renderSprite.setScale(viewportScale, viewportScale);
                this->_renderSprite.setPosition({previewX, MARGIN});

                this->applyCursor(window, this->_leaveButton.getCursor());
            }

            void draw(sf::RenderWindow &window) override
            {
                if (this->_clusterClient == nullptr)
                    return;

                this->drawPreview(window);
                this->drawInfoCard(window);
                this->_toastManager.update();
                this->_toastManager.draw(window);
            }

        private:
            float cardHeight() const
            {
                // header + address + 4 info rows + separators + button.
                return (26.f + 22.f + 4 * 22.f + 20.f + 34.f);
            }

            void drawPreview(sf::RenderWindow &window)
            {
                const float previewX = MARGIN + CARD_W + MARGIN;
                const float availW = static_cast<float>(window.getSize().x) - previewX - MARGIN;
                const float availH = static_cast<float>(window.getSize().y) - 2 * MARGIN;

                if (this->_currentRender.size_x > 0 && this->_currentRender.size_y > 0)
                {
                    updateRenderPreview();
                    window.draw(this->_renderSprite);
                    return;
                }
                // No frame to show (server is rendering, or no scene yet): a
                // placeholder keeps the panel from looking broken.
                if (availW <= 0 || availH <= 0)
                    return;
                sf::RectangleShape placeholder({availW, availH});
                placeholder.setPosition(previewX, MARGIN);
                placeholder.setFillColor(theme::BG_PANEL);
                placeholder.setOutlineThickness(1.f);
                placeholder.setOutlineColor(theme::OUTLINE_MID);
                window.draw(placeholder);

                const bool serverBusy = this->_clusterClient->getServerRenderState() == ServerRenderState::RENDERING;
                this->_line.setString(serverBusy ? "Server is rendering - helping render tiles..."
                                                  : "Waiting for scene data...");
                this->_line.setCharacterSize(14);
                this->_line.setFillColor(theme::TEXT_DIM);
                const sf::FloatRect bounds = this->_line.getLocalBounds();
                this->_line.setPosition(previewX + (availW - bounds.width) / 2.f, MARGIN + availH / 2.f - 10.f);
                window.draw(this->_line);
            }

            void drawLine(const std::string &text, float x, float y, unsigned size, const sf::Color &color,
                sf::RenderWindow &window)
            {
                this->_line.setString(text);
                this->_line.setCharacterSize(size);
                this->_line.setFillColor(color);
                this->_line.setPosition(x, y);
                window.draw(this->_line);
            }

            // A "Label            value" row; value is right-aligned in the card,
            // optionally preceded by a coloured status dot.
            void drawRow(const std::string &label, const std::string &value, const sf::Color &valueColor,
                bool withDot, float cardX, float &penY, sf::RenderWindow &window)
            {
                const float pad = 12.f;
                this->drawLine(label, cardX + pad, penY, 12, theme::TEXT_DIM, window);

                float valueRight = cardX + CARD_W - pad;
                this->_line.setString(value);
                this->_line.setCharacterSize(12);
                this->_line.setFillColor(valueColor);
                const float valueWidth = this->_line.getLocalBounds().width;
                this->_line.setPosition(valueRight - valueWidth, penY);
                window.draw(this->_line);
                if (withDot)
                {
                    sf::CircleShape dot(3.5f);
                    dot.setPosition(valueRight - valueWidth - 12.f, penY + 4.f);
                    dot.setFillColor(valueColor);
                    window.draw(dot);
                }
                penY += 22.f;
            }

            void drawInfoCard(sf::RenderWindow &window)
            {
                const float cardX = MARGIN;
                const float cardY = MARGIN;
                const float cardH = this->cardHeight();

                sf::RectangleShape card({CARD_W, cardH});
                card.setPosition(cardX, cardY);
                card.setFillColor(theme::withAlpha(theme::BG_POPUP, 240));
                card.setOutlineThickness(1.f);
                card.setOutlineColor(theme::OUTLINE_MID);
                window.draw(card);

                sf::RectangleShape header({CARD_W, 26.f});
                header.setPosition(cardX, cardY);
                header.setFillColor(theme::BG_BAR);
                window.draw(header);
                this->drawLine("CLUSTER CLIENT", cardX + 12.f, cardY + 5.f, 13, theme::TEXT_WHITE, window);

                const std::string endpoint = this->_clusterClient->getAddress() + ":"
                    + std::to_string(this->_clusterClient->getPort());
                this->drawLine(endpoint, cardX + 12.f, cardY + 30.f, 12, theme::TEXT_SUBTLE, window);

                float penY = cardY + 54.f;
                const ConnectionState conn = this->_clusterClient->getConnectionState();
                this->drawRow("Connection", connectionLabel(conn), connectionColor(conn), true, cardX, penY, window);

                const bool serverBusy = this->_clusterClient->getServerRenderState() == ServerRenderState::RENDERING;
                this->drawRow("Server", serverBusy ? "Rendering" : "Idle",
                    serverBusy ? theme::ACCENT : theme::TEXT_DIM, false, cardX, penY, window);

                this->drawRow("Activity", activityLabel(this->_clusterClient->getClientState()),
                    theme::TEXT_MAIN, false, cardX, penY, window);

                IScene *scene = this->_clusterClient->getScene();
                const std::string sceneInfo = scene
                    ? std::to_string(scene->getPrimitives().size()) + " obj - "
                        + std::to_string(scene->getLights().size()) + " lights"
                    : "-";
                this->drawRow("Scene", sceneInfo, scene ? theme::TEXT_MAIN : theme::TEXT_DIM, false, cardX, penY, window);

                window.draw(this->_leaveButton);
            }

            sf::Text _line;
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
