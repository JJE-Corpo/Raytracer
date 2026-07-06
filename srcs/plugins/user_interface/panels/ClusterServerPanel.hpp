//
// Server-side cluster HUD: a compact card drawn over the host's viewport that
// shows the listening port, the current render state and every connected client
// (name, peer address, connection state and how many tiles it has rendered).
// Header-only like the other lightweight UI pieces; owned and drawn by
// DefaultScreen whenever the local node is hosting a cluster.
//

#ifndef CLUSTERSERVERPANEL_HPP
#define CLUSTERSERVERPANEL_HPP

#include <algorithm>
#include <string>
#include <vector>
#include <SFML/Graphics.hpp>

#include "../CursorType.hpp"
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

            // Mouse routing for the panel: the header minimize button toggles the
            // body, dragging the rest of the header moves the whole panel, and any
            // click on the card is swallowed so it never falls through to the
            // scene. Returns true when the panel consumed the event.
            bool handleEvent(const sf::Event &event, sf::Vector2i mouse)
            {
                const float mx = static_cast<float>(mouse.x);
                const float my = static_cast<float>(mouse.y);

                if (event.type == sf::Event::MouseMoved)
                {
                    if (this->_dragging)
                    {
                        this->_pos = sf::Vector2f(mx - this->_dragGrab.x, my - this->_dragGrab.y);
                        this->_placed = true;
                        return (true);
                    }
                    this->_toggleHovered = this->_toggleBounds.contains(mx, my);
                    this->_headerHovered = this->_headerBounds.contains(mx, my) && !this->_toggleHovered;
                    return (false);
                }
                if (event.type == sf::Event::MouseButtonPressed
                    && event.mouseButton.button == sf::Mouse::Left)
                {
                    if (this->_toggleBounds.contains(mx, my))
                    {
                        this->_collapsed = !this->_collapsed;
                        return (true);
                    }
                    if (this->_headerBounds.contains(mx, my))
                    {
                        // Grab the header to drag the whole panel; remember where on
                        // the card we grabbed so it does not jump under the cursor.
                        this->_dragging = true;
                        this->_placed = true;
                        this->_dragGrab = sf::Vector2f(mx - this->_cardBounds.left, my - this->_cardBounds.top);
                        return (true);
                    }
                    if (this->_cardBounds.contains(mx, my))
                        return (true);
                    return (false);
                }
                if (event.type == sf::Event::MouseButtonReleased
                    && event.mouseButton.button == sf::Mouse::Left && this->_dragging)
                {
                    this->_dragging = false;
                    return (true);
                }
                return (false);
            }

            // Cursor the host should show: a 4-way move over the draggable header
            // (or mid-drag), a hand over the minimize button, arrow otherwise.
            CursorType getCursor() const
            {
                if (this->_dragging || this->_headerHovered)
                    return (CursorType::MOVE);
                if (this->_toggleHovered)
                    return (CursorType::HAND);
                return (CursorType::ARROW);
            }

            // Anchored by its top-right corner at (rightEdge, top) so it hugs the
            // viewport's top-right. leftBound is the left edge of the content pane
            // (the sidebar's right edge), so the panel can never cover the sidebar.
            void draw(sf::RenderTarget &window, const IClusterServer *server,
                bool rendering, int currentSample, float rightEdge, float top, float leftBound = 0.f)
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
                const float fullHeight = headerH + bodyH + pad;
                const float height = this->_collapsed ? headerH : fullHeight;

                // Anchored to the top-right until the user drags it; once moved it
                // keeps its own position. Clamp so it always stays on-screen (also
                // after a window resize or dragging toward an edge).
                const float winW = static_cast<float>(window.getSize().x);
                const float winH = static_cast<float>(window.getSize().y);
                const float maxX = std::max(leftBound, winW - width);
                const float maxY = std::max(top, winH - height);
                float x = this->_placed ? this->_pos.x : (rightEdge - width - 12.f);
                float y = this->_placed ? this->_pos.y : top;
                x = std::max(leftBound, std::min(x, maxX));
                y = std::max(top, std::min(y, maxY));
                if (this->_placed)
                    this->_pos = sf::Vector2f(x, y);

                this->_cardBounds = sf::FloatRect(x, y, width, height);
                this->_headerBounds = sf::FloatRect(x, y, width, headerH);

                sf::RectangleShape card({width, height});
                card.setPosition(x, y);
                card.setFillColor(theme::withAlpha(theme::BG_POPUP, 235));
                card.setOutlineThickness(1.f);
                card.setOutlineColor(theme::OUTLINE_MID);
                window.draw(card);

                sf::RectangleShape header({width, headerH});
                header.setPosition(x, y);
                header.setFillColor((this->_headerHovered || this->_dragging) ? theme::BG_HOVER : theme::BG_BAR);
                window.draw(header);

                sf::CircleShape statusDot(4.f);
                statusDot.setPosition(x + pad, y + headerH / 2.f - 4.f);
                statusDot.setFillColor(rendering ? theme::ACCENT : theme::CHECKED);
                window.draw(statusDot);

                // Minimize / expand button on the header's right edge.
                const float btnSize = 16.f;
                const float btnX = x + width - pad - btnSize;
                const float btnY = y + (headerH - btnSize) / 2.f;
                this->_toggleBounds = sf::FloatRect(btnX, btnY, btnSize, btnSize);
                sf::RectangleShape toggle({btnSize, btnSize});
                toggle.setPosition(btnX, btnY);
                toggle.setFillColor(this->_toggleHovered ? theme::BG_CONTROL_HOVER : theme::BG_CONTROL);
                window.draw(toggle);
                // Glyph: "-" when open (click to minimize), "+" when collapsed.
                this->drawLine(this->_collapsed ? "+" : "-", btnX + (this->_collapsed ? 4.f : 5.f), btnY - 2.f,
                    15, theme::TEXT_WHITE, window);

                std::string title = "CLUSTER SERVER";
                if (this->_collapsed)
                    title += "  (" + std::to_string(connected) + ")";
                this->drawLine(title, x + pad + 14.f, y + 5.f, 13, theme::TEXT_WHITE, window);

                if (this->_collapsed)
                    return;

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
            bool _collapsed = false;
            bool _toggleHovered = false;
            bool _headerHovered = false;
            sf::FloatRect _toggleBounds;
            sf::FloatRect _headerBounds;
            sf::FloatRect _cardBounds;

            // Free-drag position (top-left). Anchored top-right until _placed.
            bool _placed = false;
            bool _dragging = false;
            sf::Vector2f _pos;
            sf::Vector2f _dragGrab;
    };
}

#endif
