//
// Server-side cluster HUD: a compact card drawn over the host's viewport that
// shows the listening port, the current render state and every connected client
// (name, peer address, connection state and how many tiles it has rendered).
// Header-only like the other lightweight UI pieces; owned and drawn by
// DefaultScreen whenever the local node is hosting a cluster.
//

#ifndef CLUSTERSERVERPANEL_HPP
#define CLUSTERSERVERPANEL_HPP

#include <string>
#include <vector>
#include <SFML/Graphics.hpp>

#include "../Theme.hpp"
#include "../../../common/cluster/IClusterServer.hpp"

namespace rc
{
    class ClusterServerPanel
    {
        public:
            void setFont(sf::Font &font)
            {
                this->_font = &font;
                this->_text.setFont(font);
            }

            // Anchored by its top-right corner at (rightEdge, top) so it hugs the
            // viewport's top-right without overlapping the left sidebar.
            void draw(sf::RenderTarget &window, const IClusterServer *server,
                bool rendering, int currentSample, float rightEdge, float top)
            {
                if (this->_font == nullptr || server == nullptr)
                    return;

                const std::vector<IClusterServer::ClientInfo> clients = server->getClients();
                size_t connected = 0;
                for (const auto &client : clients)
                    if (client.state == ConnectionState::CONNECTED)
                        ++connected;

                const float width = 260.f;
                const float pad = 12.f;
                const float rowH = 18.f;
                const float headerH = 26.f;
                const int clientRows = clients.empty() ? 1 : static_cast<int>(clients.size());
                const float bodyH = 4 * rowH + 6.f + clientRows * rowH;
                const float height = headerH + bodyH + pad;
                const float x = rightEdge - width - 12.f;
                const float y = top;

                sf::RectangleShape card({width, height});
                card.setPosition(x, y);
                card.setFillColor(theme::withAlpha(theme::BG_POPUP, 235));
                card.setOutlineThickness(1.f);
                card.setOutlineColor(theme::OUTLINE_MID);
                window.draw(card);

                sf::RectangleShape header({width, headerH});
                header.setPosition(x, y);
                header.setFillColor(theme::BG_BAR);
                window.draw(header);

                sf::CircleShape statusDot(4.f);
                statusDot.setPosition(x + pad, y + headerH / 2.f - 4.f);
                statusDot.setFillColor(rendering ? theme::ACCENT : theme::CHECKED);
                window.draw(statusDot);

                this->drawLine("CLUSTER SERVER", x + pad + 14.f, y + 5.f, 13, theme::TEXT_WHITE, window);

                LayoutY pen{y + headerH + 8.f};
                this->drawRow("Port", std::to_string(server->getPort()), x, width, pad, pen, theme::TEXT_MAIN, window);
                if (rendering)
                {
                    const std::string sample = currentSample >= 0
                        ? "Rendering - sample " + std::to_string(currentSample)
                        : "Rendering";
                    this->drawRow("State", sample, x, width, pad, pen, theme::ACCENT, window);
                }
                else
                {
                    this->drawRow("State", "Idle", x, width, pad, pen, theme::TEXT_DIM, window);
                }
                this->drawRow("Clients", std::to_string(connected) + " connected", x, width, pad, pen, theme::TEXT_MAIN, window);

                pen.y += 4.f;
                sf::RectangleShape sep({width - 2 * pad, 1.f});
                sep.setPosition(x + pad, pen.y);
                sep.setFillColor(theme::OUTLINE_MID);
                window.draw(sep);
                pen.y += 6.f;

                if (clients.empty())
                {
                    this->drawLine("Waiting for clients...", x + pad, pen.y, 12, theme::TEXT_DIM, window);
                    pen.y += rowH;
                    return;
                }
                for (const auto &client : clients)
                    this->drawClientRow(client, x, width, pad, pen, window);
            }

        private:
            struct LayoutY { float y; };

            static sf::Color stateColor(ConnectionState state)
            {
                switch (state)
                {
                    case ConnectionState::CONNECTED: return (theme::CHECKED);
                    case ConnectionState::PENDING:   return (theme::ACCENT);
                    default:                         return (theme::TOAST_ERROR);
                }
            }

            static const char *stateLabel(ConnectionState state)
            {
                switch (state)
                {
                    case ConnectionState::CONNECTED:    return ("connected");
                    case ConnectionState::PENDING:      return ("pending");
                    case ConnectionState::REFUSED:      return ("refused");
                    default:                            return ("offline");
                }
            }

            void drawLine(const std::string &text, float x, float y, unsigned size, const sf::Color &color,
                sf::RenderTarget &window)
            {
                this->_text.setString(text);
                this->_text.setCharacterSize(size);
                this->_text.setFillColor(color);
                this->_text.setPosition(x, y);
                window.draw(this->_text);
            }

            // A "Label            value" row with the label dimmed on the left and
            // the value coloured on the right edge of the card.
            void drawRow(const std::string &label, const std::string &value, float x, float width, float pad,
                LayoutY &pen, const sf::Color &valueColor, sf::RenderTarget &window)
            {
                this->drawLine(label, x + pad, pen.y, 12, theme::TEXT_DIM, window);

                this->_text.setString(value);
                this->_text.setCharacterSize(12);
                this->_text.setFillColor(valueColor);
                const float valueWidth = this->_text.getLocalBounds().width;
                this->_text.setPosition(x + width - pad - valueWidth, pen.y);
                window.draw(this->_text);
                pen.y += 18.f;
            }

            void drawClientRow(const IClusterServer::ClientInfo &client, float x, float width, float pad,
                LayoutY &pen, sf::RenderTarget &window)
            {
                sf::CircleShape dot(3.5f);
                dot.setPosition(x + pad, pen.y + 4.f);
                dot.setFillColor(stateColor(client.state));
                window.draw(dot);

                std::string label = client.name;
                if (client.address != "?" && !client.address.empty())
                    label += " (" + client.address + ")";
                this->drawLine(label, x + pad + 12.f, pen.y, 12, theme::TEXT_SUBTLE, window);

                // Connected clients show their tile contribution; others show why
                // they are not rendering yet (pending handshake, refused, ...).
                std::string right;
                sf::Color rightColor;
                if (client.state == ConnectionState::CONNECTED)
                {
                    right = std::to_string(client.tilesRendered) + " tiles";
                    rightColor = theme::TEXT_DIM;
                }
                else
                {
                    right = stateLabel(client.state);
                    rightColor = stateColor(client.state);
                }
                this->_text.setString(right);
                this->_text.setCharacterSize(11);
                this->_text.setFillColor(rightColor);
                const float rightWidth = this->_text.getLocalBounds().width;
                this->_text.setPosition(x + width - pad - rightWidth, pen.y + 1.f);
                window.draw(this->_text);
                pen.y += 18.f;
            }

            sf::Font *_font = nullptr;
            sf::Text _text;
    };
}

#endif
