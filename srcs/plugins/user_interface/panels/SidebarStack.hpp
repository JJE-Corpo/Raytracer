//
// Created by jazema on 7/1/26.
//
// Owns the stack of collapsible/scrollable sidebar sections and the draggable
// dividers between them. Each frame it decides which sections are visible (from
// the current selection), distributes the available height by per-section weight
// (persisted across selection changes), places sections + dividers, draws them
// (clipped bodies -> divider lines -> unclipped pop-up overlay pass) and exposes
// its interactive components to the EventRouter.
//

#ifndef SIDEBARSTACK_HPP
#define SIDEBARSTACK_HPP

#include <algorithm>
#include <map>
#include <string>
#include <vector>

#include <SFML/Graphics.hpp>

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

            Section &section(Slot slot)
            {
                return (this->_sections[slot]);
            }

            void setVisible(Slot slot, bool visible)
            {
                this->_visibleFlag[slot] = visible;
            }

            // Distribute [top, bottom] over the visible sections and place dividers.
            void layout(float width, float top, float bottom)
            {
                this->_order.clear();
                for (int i = 0; i < SLOT_COUNT; ++i)
                    if (this->_visibleFlag[i])
                        this->_order.push_back(&this->_sections[i]);

                const std::size_t count = this->_order.size();
                const float availableH = std::max(0.f, bottom - top);
                const float bodyPool = std::max(0.f, availableH - static_cast<float>(count) * Section::HEADER_H);

                float sumW = 0.f;
                std::size_t expandedCount = 0;
                for (Section *s : this->_order)
                {
                    if (!s->collapsed)
                    {
                        sumW += this->weightRef(s->id);
                        ++expandedCount;
                    }
                }

                const float extra = std::max(0.f, bodyPool - static_cast<float>(expandedCount) * MIN_BODY);

                float y = top;
                for (std::size_t i = 0; i < count; ++i)
                {
                    Section *s = this->_order[i];
                    float bodyH = 0.f;
                    if (!s->collapsed && sumW > 0.f)
                        bodyH = MIN_BODY + extra * (this->weightRef(s->id) / sumW);
                    s->place(0.f, y, width, bodyH);
                    y = s->bodyBottom();
                }

                this->layoutDividers(width);
            }

            void update(sf::Vector2i mouse)
            {
                for (std::size_t i = 0; i + 1 < this->_order.size(); ++i)
                    if (this->_dividers[i].enabled)
                        this->_dividers[i].update(mouse);
                for (Section *s : this->_order)
                    s->update(mouse);
            }

            void draw(sf::RenderTarget &target)
            {
                for (Section *s : this->_order)
                    target.draw(*s);
                for (std::size_t i = 0; i + 1 < this->_order.size(); ++i)
                    target.draw(this->_dividers[i]);
                // Unclipped overlay pass for open pop-ups (dropdown / color picker).
                for (Section *s : this->_order)
                    s->drawOverlay(target, sf::RenderStates::Default);
            }

            void collectComponents(std::vector<Component *> &out)
            {
                for (Section *s : this->_order)
                    out.push_back(s);
                for (std::size_t i = 0; i + 1 < this->_order.size(); ++i)
                    if (this->_dividers[i].enabled)
                        out.push_back(&this->_dividers[i]);
            }

            CursorType getCursor()
            {
                for (std::size_t i = 0; i + 1 < this->_order.size(); ++i)
                    if (this->_dividers[i].enabled && this->_dividers[i].getCursor() != CursorType::ARROW)
                        return (this->_dividers[i].getCursor());
                for (Section *s : this->_order)
                    if (s->getCursor() != CursorType::ARROW)
                        return (s->getCursor());
                return (CursorType::ARROW);
            }

        private:
            Section _sections[SLOT_COUNT];
            bool _visibleFlag[SLOT_COUNT] = {true, false, false, false};
            ResizeHandleV _dividers[SLOT_COUNT - 1];
            std::vector<Section *> _order;
            std::map<std::string, float> _weight;

            float &weightRef(const std::string &id)
            {
                auto it = this->_weight.find(id);
                if (it == this->_weight.end())
                    it = this->_weight.emplace(id, 1.f).first;
                return (it->second);
            }

            void layoutDividers(float width)
            {
                for (std::size_t i = 0; i + 1 < this->_order.size(); ++i)
                {
                    Section *a = this->_order[i];
                    Section *b = this->_order[i + 1];
                    ResizeHandleV &divider = this->_dividers[i];

                    const bool interactive = !a->collapsed && !b->collapsed;
                    divider.enabled = interactive;
                    divider.setBounds(a->bodyBottom(), 0.f, width);

                    if (!interactive)
                        continue;

                    const float aTop = a->bodyTop();
                    const float pairPixels = a->bodyHeight() + b->bodyHeight();
                    divider.setRange(aTop + MIN_BODY, a->bodyBottom() + b->bodyHeight() - MIN_BODY);

                    const std::string idA = a->id;
                    const std::string idB = b->id;
                    divider.onResize = [this, idA, idB, aTop, pairPixels](float newY)
                    {
                        if (pairPixels <= 0.f)
                            return;
                        const float newABody = std::clamp(newY - aTop, MIN_BODY, pairPixels - MIN_BODY);
                        const float p = newABody / pairPixels;
                        const float combined = this->weightRef(idA) + this->weightRef(idB);
                        this->weightRef(idA) = combined * p;
                        this->weightRef(idB) = combined * (1.f - p);
                    };
                }
            }
    };
}

#endif
