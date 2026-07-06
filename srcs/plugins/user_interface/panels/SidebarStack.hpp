//
// Created by jazema on 7/1/26.
//

#ifndef SIDEBARSTACK_HPP
#define SIDEBARSTACK_HPP

#include <map>
#include <string>
#include <vector>

#include "Section.hpp"
#include "../Component.hpp"
#include "../components/ResizeHandleV.hpp"

namespace rc
{
    class SidebarStack
    {
        public:
            // Section slots, in stacking order.
            enum Slot { HIERARCHY = 0, CAMERA = 1, OBJECT = 2, MATERIAL = 3, SLOT_COUNT = 4 };

            static constexpr float MIN_BODY = 48.f;

            Section &section(Slot slot);
            void setVisible(Slot slot, bool visible);

            void layout(float width, float top, float bottom);

            void update(sf::Vector2i mouse);
            void draw(sf::RenderTarget &target);
            void collectComponents(std::vector<Component *> &out);
            CursorType getCursor();

        private:
            Section _sections[SLOT_COUNT];
            bool _visibleFlag[SLOT_COUNT] = {true, false, false, false};
            ResizeHandleV _dividers[SLOT_COUNT - 1];
            std::vector<Section *> _order;
            std::map<std::string, float> _weight;

            float &weightRef(const std::string &id);
            void layoutDividers(float width);
    };
}

#endif
