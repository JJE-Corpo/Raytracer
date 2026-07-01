//
// Created by jazema on 5/14/26.
//

#ifndef RENDERERPANEL_HPP
#define RENDERERPANEL_HPP
#include "../components/Button.hpp"

namespace rc
{
    struct RendererPanel : Component
    {
        mutable sf::IntRect viewportBounds = {0, 0, 0, 0};
        mutable float viewportScale = 1.0f;
        std::function<void()> closeRenderCallback = [] {};
        std::function<bool()> isRenderViewCallback = [] {return (false);};
        std::function<std::string()> getRendererCommentCallback = []{return ("");};

        void setFont(sf::Font &font) override
        {
            this->_closeRenderButton.setFont(font);
            this->_closeRenderButton.setLabel("X");

            this->_renderingText.setCharacterSize(14);
            this->_renderingText.setFillColor(theme::TEXT_WHITE);
            this->_renderingText.setFont(font);
        }

        void update(sf::Vector2i mouse) override
        {
            this->_closeRenderButton.update(mouse);
        }

        void layout(float x, float y, float width, float height)
        {
            VerticalLayout layout{x,y,0};

            this->_tabBar.setPosition(layout.x, layout.y);
            this->_tabBar.setSize({width, 28});
            this->_tabBar.setFillColor(theme::BG_WINDOW);
            this->_renderingText.setPosition(layout.x + 10, layout.y + 6);
            this->_closeRenderButton.layout(layout.x + width - 30, layout.y + 6, 24, 18);
            this->_closeRenderButton.onClick = closeRenderCallback;
            layout.next(28);

            this->_availableViewportSize = {width, height - layout.y};

            this->_renderSprite.setPosition({layout.x, layout.y});
        }

        void updateRender(const Render &render)
        {
            this->_currentRender = render;
        }

        bool getViewportPixel(const sf::Vector2i &mouse, sf::Vector2i &pixel) const
        {
            if (this->viewportScale <= 0.0f)
                return (false);
            if (!this->viewportBounds.contains(mouse.x, mouse.y))
                return (false);

            const float local_x = static_cast<float>(mouse.x) - static_cast<float>(this->viewportBounds.left);
            const float local_y = static_cast<float>(mouse.y) - static_cast<float>(this->viewportBounds.top);

            const int px = static_cast<int>(local_x / this->viewportScale);
            const int py = static_cast<int>(local_y / this->viewportScale);
            if (px < 0 || py < 0 || px >= static_cast<int>(this->viewportBounds.width) || py >= static_cast<int>(this->viewportBounds.height))
                return (false);

            pixel = {px, py};
            return (true);
        }

        void handleEvent(const sf::Event &event, const sf::Vector2i mouse) override
        {
            this->_closeRenderButton.handleEvent(event, mouse);
        }

        void draw(sf::RenderTarget &target, sf::RenderStates states) const override
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

            const float scaleX = this->_availableViewportSize.x / static_cast<float>(this->_currentRender.size_x);
            const float scaleY = this->_availableViewportSize.y / static_cast<float>(this->_currentRender.size_y);
            this->viewportScale = std::min(scaleX, scaleY);

            this->viewportBounds = {
                static_cast<int>(this->_renderSprite.getPosition().x),
                static_cast<int>(this->_renderSprite.getPosition().y),
                static_cast<int>(static_cast<float>(this->_currentRender.size_x) * this->viewportScale),
                static_cast<int>(static_cast<float>(this->_currentRender.size_y) * this->viewportScale)
            };

            this->_renderSprite.setScale(this->viewportScale, this->viewportScale);

            target.draw(this->_tabBar, states);

            if (this->isRenderViewCallback())
            {
                this->_renderingText.setString(this->getRendererCommentCallback());
                target.draw(this->_renderingText, states);

                target.draw(this->_closeRenderButton, states);
            }

            target.draw(this->_renderSprite, states);
        }

        CursorType getCursor() override
        {
            return (this->_closeRenderButton.getCursor());
        }

        private:
            sf::RectangleShape _tabBar;
            mutable sf::Text _renderingText;
            Button _closeRenderButton;

            mutable sf::Texture _renderTexture;
            mutable sf::Sprite _renderSprite;
            sf::Vector2f _availableViewportSize = {0, 0};
            Render _currentRender = {0, 0, {}};
    };
}

#endif
