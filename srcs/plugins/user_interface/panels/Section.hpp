//
// Created by jazema on 7/1/26.
//
// A sidebar section: a fixed header on top of a scrollable body (ScrollView)
// wrapping one panel. The SidebarStack owns a list of these, sizes them, and
// puts draggable dividers between them.
//

#ifndef SECTION_HPP
#define SECTION_HPP

#include <string>

#include "../Component.hpp"
#include "../components/ScrollView.hpp"

namespace rc
{
    class Section : public Component
    {
        public:
            std::string id;
            ScrollView body;

            static constexpr float HEADER_H = 26.f;
            static constexpr float PAD_X = 14.f;  // horizontal inset of the body content
            static constexpr float PAD_Y = 8.f;   // gap under the header

            // Per-section override of the body's horizontal inset (defaults to
            // PAD_X). Set to 0 for panels that want edge-to-edge rows.
            float padX = PAD_X;

            void setFont(sf::Font &font) override;
            void setTitle(const std::string &title);

            // Position the section: header of fixed height on top, body filling
            // bodyHeight below it. Called every frame by the stack.
            void place(float x, float y, float width, float bodyHeight);

            float bodyBottom() const;
            float bodyTop() const;
            float bodyHeight() const;

            sf::FloatRect getBounds() const override;
            bool isCapturing() const override;
            void update(sf::Vector2i mouse) override;
            bool handleEvent(const sf::Event &event, sf::Vector2i mouse) override;
            void draw(sf::RenderTarget &target, sf::RenderStates states) const override;
            void drawOverlay(sf::RenderTarget &target, sf::RenderStates states) const override;
            CursorType getCursor() override;

        private:
            sf::Text _title;
            sf::FloatRect _fullRect;
            sf::FloatRect _headerRect;
    };
}

#endif
