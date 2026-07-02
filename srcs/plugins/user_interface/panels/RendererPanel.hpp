//
// Created by jazema on 5/14/26.
//

#ifndef RENDERERPANEL_HPP
#define RENDERERPANEL_HPP

#include "../components/Button.hpp"
#include "../../../common/ISceneRenderer.hpp"

namespace rc
{
    class RendererPanel : public Component
    {
        public:
            mutable sf::IntRect viewportBounds = {0, 0, 0, 0};
            mutable float viewportScale = 1.0f;
            std::function<void()> closeRenderCallback = [] {};
            std::function<bool()> isRenderViewCallback = [] { return (false); };
            std::function<std::string()> getRendererCommentCallback = [] { return (""); };

            void setFont(sf::Font &font) override;
            void layout(float x, float y, float width, float height);
            void updateRender(const Render &render);

            // Converts a window-space mouse position into a render pixel, false if
            // it falls outside the currently laid-out viewport.
            bool getViewportPixel(const sf::Vector2i &mouse, sf::Vector2i &pixel) const;

            void update(sf::Vector2i mouse) override;
            bool handleEvent(const sf::Event &event, sf::Vector2i mouse) override;
            void draw(sf::RenderTarget &target, sf::RenderStates states) const override;
            CursorType getCursor() override;

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
