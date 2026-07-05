//
// Created by the materials market feature.
//
// A floating, threaded window that browses the materials market
// (~/.raytracer/materials). Every material created or loaded is mirrored there
// by the core; this window lists them and lets the user pull any of them back
// into the current scene's material set. Like LoadWindow it runs its own SFML
// window on a worker thread and never touches the scene directly: the "Add"
// clicks are queued and drained by DefaultScreen on the main thread, so the
// scene is only ever mutated from one thread.
//
// Clicking a row toggles an inline, scrollable details panel (accordion) that
// lists every property of that material.
//

#ifndef MARKETWINDOW_HPP
#define MARKETWINDOW_HPP

#include <algorithm>
#include <cctype>
#include <cstddef>
#include <cstdio>
#include <mutex>
#include <string>
#include <thread>
#include <utility>
#include <vector>
#include <SFML/Graphics.hpp>

#include "../../../common/Material.hpp"
#include "../../../common/MaterialLibrary.hpp"
#include "../Theme.hpp"
#include "../LayoutPen.hpp"
#include "../components/Button.hpp"
#include "../components/TextField.hpp"
#include "Window.hpp"

namespace rc
{
    struct MarketWindow : Window
    {
        std::vector<Material> marketMaterials;
        std::vector<Button> addButtons;
        std::vector<bool> added;
        Button refreshButton;

        // Search bar: only materials whose name contains the (case-insensitive)
        // query are shown. `visibleIndices` maps rows to marketMaterials slots.
        TextField searchField;
        std::vector<std::size_t> visibleIndices;
        // Top Y (window space, scrolled) of each visible row, for hit-testing a
        // click on the row body. Parallel to visibleIndices.
        std::vector<float> rowTops;

        // marketMaterials index whose details are expanded, or -1 for none.
        int expandedIndex = -1;

        // "Add" clicks land here; DefaultScreen drains them on the main thread.
        std::mutex actionMutex;
        std::vector<Material> pendingAdds;

        float scrollY = 0.f;
        float contentHeight = 0.f;

        static constexpr float ROW_HEIGHT = 54.f;
        static constexpr float HEADER_HEIGHT = 90.f;
        static constexpr float ADD_BUTTON_WIDTH = 90.f;
        static constexpr float DETAIL_LINE_H = 18.f;
        static constexpr float DETAIL_PAD_TOP = 8.f;
        static constexpr float DETAIL_PAD_BOTTOM = 12.f;

        void create(sf::Font &f)
        {
            font = &f;
            windowWidth = 460;
            windowHeight = 520;
            windowTitle = "Material Market";

            refreshButton.setFont(*font);
            refreshButton.setLabel("Refresh");
            refreshButton.onClick = [this] { this->reload(); };

            searchField.setFont(*font);
            searchField.setCharacterSize(14);

            this->reload();

            running = true;
            thread = std::thread(&MarketWindow::loop, this);
        }

        // Pull the queued additions out for the main thread to apply.
        void drainPendingAdds(std::vector<Material> &out)
        {
            std::lock_guard<std::mutex> lock(this->actionMutex);
            if (this->pendingAdds.empty())
                return;
            out.insert(out.end(), this->pendingAdds.begin(), this->pendingAdds.end());
            this->pendingAdds.clear();
        }

        // Re-read the market folder and rebuild one "Add" button per material.
        void reload()
        {
            this->marketMaterials = MaterialLibrary::loadAll();
            this->added.assign(this->marketMaterials.size(), false);
            this->addButtons.clear();
            for (std::size_t i = 0; i < this->marketMaterials.size(); ++i)
            {
                Button button;
                button.setFont(*font);
                button.setLabel("Add");
                button.onClick = [this, i] { this->queueAdd(i); };
                this->addButtons.push_back(button);
            }
            this->expandedIndex = -1;
            this->scrollY = 0.f;
            this->computeVisible();
        }

        void queueAdd(std::size_t index)
        {
            if (index >= this->marketMaterials.size())
                return;
            {
                std::lock_guard<std::mutex> lock(this->actionMutex);
                this->pendingAdds.push_back(this->marketMaterials[index]);
            }
            if (index < this->added.size())
                this->added[index] = true;
            if (index < this->addButtons.size())
            {
                this->addButtons[index].setLabel("Added");
                this->addButtons[index].enabled = false;
            }
        }

        static std::string toLower(const std::string &s)
        {
            std::string out = s;
            std::transform(out.begin(), out.end(), out.begin(), [](unsigned char c) {
                return static_cast<char>(std::tolower(c));
            });
            return (out);
        }

        // Rebuild visibleIndices from the current search query.
        void computeVisible()
        {
            const std::string query = toLower(this->searchField.value);

            this->visibleIndices.clear();
            for (std::size_t i = 0; i < this->marketMaterials.size(); ++i)
            {
                if (query.empty() || toLower(this->marketMaterials[i].name).find(query) != std::string::npos)
                    this->visibleIndices.push_back(i);
            }
        }

        static std::string fmt(float v)
        {
            char buf[32];
            std::snprintf(buf, sizeof(buf), "%.2f", v);
            return (std::string(buf));
        }

        static std::string colorText(const ColorF &cf)
        {
            const Color c = cf.toColor();
            return (std::to_string(c.r) + ", " + std::to_string(c.g) + ", " + std::to_string(c.b));
        }

        static std::string materialProps(const Material &m)
        {
            if (m.model == MaterialModel::PBR)
                return ("metallic " + fmt(m.metallic) + "   roughness " + fmt(m.roughness) + "   ior " + fmt(m.ior));
            return ("shininess " + fmt(m.shininess) + "   refl " + fmt(m.reflectivity) + "   transp " + fmt(m.transparency));
        }

        // The (label, value) rows shown in an expanded material's details panel.
        // The list is the same length for every material, so the panel height is
        // constant.
        static std::vector<std::pair<std::string, std::string>> detailLines(const Material &m)
        {
            std::string model = m.getModelName();
            std::transform(model.begin(), model.end(), model.begin(), ::toupper);

            return {
                {"Model", model},
                {"Base color", colorText(m.baseColor)},
                {"Specular", colorText(m.specular)},
                {"Shininess", fmt(m.shininess)},
                {"Reflectivity", fmt(m.reflectivity)},
                {"Transparency", fmt(m.transparency)},
                {"IOR", fmt(m.ior)},
                {"Metallic", fmt(m.metallic)},
                {"Roughness", fmt(m.roughness)},
                {"AO", fmt(m.ao)},
                {"Specular level", fmt(m.specular_level)},
                {"Specular tint", fmt(m.specular_tint)},
                {"Clearcoat", fmt(m.clearcoat)},
                {"Clearcoat rough.", fmt(m.clearcoat_roughness)},
                {"Sheen", fmt(m.sheen)},
                {"Sheen tint", fmt(m.sheen_tint)},
                {"Transmission", fmt(m.transmission)},
                {"Alpha", fmt(m.alpha)},
                {"Normal map", m.normal_map_enabled ? (m.normal_map.empty() ? "on" : m.normal_map) : "off"},
                {"Normal scale", fmt(m.normal_scale)},
                {"Normal noise", fmt(m.normal_noise_frequency)},
            };
        }

        static float detailHeight(const Material &m)
        {
            return (static_cast<float>(detailLines(m).size()) * DETAIL_LINE_H + DETAIL_PAD_TOP + DETAIL_PAD_BOTTOM);
        }

        // Full height of a row: its header, plus the details panel when expanded.
        float rowHeight(std::size_t idx) const
        {
            float h = ROW_HEIGHT;
            if (static_cast<int>(idx) == this->expandedIndex && idx < this->marketMaterials.size())
                h += detailHeight(this->marketMaterials[idx]);
            return (h);
        }

        void drawColoredText(const std::string &text, float x, float y, unsigned int size, const sf::Color &color)
        {
            sf::Text t;
            t.setFont(*font);
            t.setString(text);
            t.setCharacterSize(size);
            t.setFillColor(color);
            t.setPosition({x, y});
            window.draw(t);
        }

        void drawRect(float x, float y, float w, float h, const sf::Color &fill, float outline = 0.f, const sf::Color &outlineColor = theme::OUTLINE)
        {
            sf::RectangleShape r;
            r.setPosition({x, y});
            r.setSize({w, h});
            r.setFillColor(fill);
            if (outline > 0.f)
            {
                r.setOutlineThickness(outline);
                r.setOutlineColor(outlineColor);
            }
            window.draw(r);
        }

        float minScroll() const
        {
            const float visible = static_cast<float>(windowHeight) - HEADER_HEIGHT;
            if (this->contentHeight <= visible)
                return (0.f);
            return (visible - this->contentHeight);
        }

        void layoutRows()
        {
            this->computeVisible();

            const float start = HEADER_HEIGHT + 10.f + this->scrollY;
            float y = start;

            this->rowTops.clear();
            for (std::size_t idx : this->visibleIndices)
            {
                this->rowTops.push_back(y);
                if (idx < this->addButtons.size())
                    this->addButtons[idx].layout(static_cast<float>(windowWidth) - 20.f - ADD_BUTTON_WIDTH, y + 8.f, ADD_BUTTON_WIDTH, 30.f);
                y += this->rowHeight(idx);
            }
            this->contentHeight = y - start;
        }

        void drawDetails(const Material &m, float x, float y, float width)
        {
            const auto lines = detailLines(m);

            drawRect(x, y, width, detailHeight(m), theme::BG_PANEL);
            drawRect(x, y, 3.f, detailHeight(m), theme::ACCENT);

            float ly = y + DETAIL_PAD_TOP;
            for (const auto &line : lines)
            {
                drawColoredText(line.first, x + 14.f, ly, 12, theme::TEXT_DIM);
                drawColoredText(line.second, x + 160.f, ly, 12, theme::TEXT_MAIN);
                ly += DETAIL_LINE_H;
            }
        }

        void drawHeader()
        {
            drawRect(0.f, 0.f, static_cast<float>(windowWidth), HEADER_HEIGHT - 2.f, theme::BG_BAR);
            drawRect(0.f, HEADER_HEIGHT - 2.f, static_cast<float>(windowWidth), 2.f, theme::ACCENT);
            drawColoredText("Material Market", 20.f, 12.f, 20, theme::TEXT_WHITE);

            refreshButton.layout(static_cast<float>(windowWidth) - 20.f - ADD_BUTTON_WIDTH, 12.f, ADD_BUTTON_WIDTH, 28.f);
            window.draw(refreshButton);

            searchField.layout(20.f, 50.f, static_cast<float>(windowWidth) - 40.f, 28.f);
            window.draw(searchField);
            // Placeholder hint shown only while the field is empty and unfocused.
            if (searchField.value.empty() && !searchField.focused)
                drawColoredText("Search materials...", 30.f, 55.f, 13, theme::TEXT_DIM);
        }

        void drawUi() override
        {
            this->layoutRows();

            const float x0 = 20.f;
            float y = HEADER_HEIGHT + 10.f + this->scrollY;

            if (this->marketMaterials.empty())
            {
                drawColoredText("No saved materials yet.", x0, y + 4.f, 14, theme::TEXT_DIM);
                drawColoredText("Create or load a material and it will appear here.", x0, y + 26.f, 12, theme::TEXT_DIM);
            }
            else if (this->visibleIndices.empty())
            {
                drawColoredText("No material matches \"" + searchField.value + "\".", x0, y + 4.f, 14, theme::TEXT_DIM);
            }

            for (std::size_t idx : this->visibleIndices)
            {
                const Material &m = this->marketMaterials[idx];
                const Color c = m.getBaseColor().toColor();
                const bool expanded = static_cast<int>(idx) == this->expandedIndex;

                if (expanded)
                    drawRect(0.f, y, static_cast<float>(windowWidth), ROW_HEIGHT, theme::BG_ITEM);

                drawRect(x0, y + 6.f, 24.f, 24.f, sf::Color(c.r, c.g, c.b), 1.f, theme::OUTLINE_MID);
                drawColoredText(m.getName(), x0 + 36.f, y + 4.f, 15, theme::TEXT_MAIN);

                std::string model = m.getModelName();
                std::transform(model.begin(), model.end(), model.begin(), ::toupper);
                drawColoredText(model, x0 + 36.f, y + 26.f, 11, theme::ACCENT);
                drawColoredText(materialProps(m), x0 + 90.f, y + 26.f, 11, theme::TEXT_DIM);

                if (idx < this->addButtons.size())
                    window.draw(this->addButtons[idx]);

                if (expanded)
                    this->drawDetails(m, 12.f, y + ROW_HEIGHT, static_cast<float>(windowWidth) - 24.f);

                y += this->rowHeight(idx);
            }

            // Opaque header drawn last so scrolled rows slide underneath it.
            this->drawHeader();
        }

        void updateUi() override
        {
            sf::Vector2i mouse = sf::Mouse::getPosition(window);

            searchField.update(mouse);
            refreshButton.update(mouse);
            for (std::size_t idx : this->visibleIndices)
            {
                if (idx >= this->addButtons.size())
                    continue;
                // Rows scrolled up behind the header must not react to hover.
                if (this->addButtons[idx].getBounds().top < HEADER_HEIGHT)
                    this->addButtons[idx].hovered = false;
                else
                    this->addButtons[idx].update(mouse);
            }
        }

        // Toggle the details panel of the row whose body (left of the Add button)
        // is under the cursor. Returns true when a row was hit.
        bool toggleRowAt(const sf::Vector2i &mouse)
        {
            const float buttonLeft = static_cast<float>(windowWidth) - 20.f - ADD_BUTTON_WIDTH;

            if (mouse.x < 20.f || mouse.x >= buttonLeft || mouse.y < HEADER_HEIGHT)
                return (false);
            for (std::size_t k = 0; k < this->visibleIndices.size() && k < this->rowTops.size(); ++k)
            {
                const float top = this->rowTops[k];
                if (mouse.y >= top && mouse.y < top + ROW_HEIGHT)
                {
                    const int idx = static_cast<int>(this->visibleIndices[k]);
                    this->expandedIndex = (this->expandedIndex == idx) ? -1 : idx;
                    return (true);
                }
            }
            return (false);
        }

        void handleEvent(const sf::Event &event) override
        {
            sf::Vector2i mouse = sf::Mouse::getPosition(window);

            // The search field gets first pick so keystrokes go to it while focused.
            searchField.handleEvent(event, mouse);

            if (event.type == sf::Event::MouseWheelScrolled)
            {
                this->scrollY = std::clamp(this->scrollY + event.mouseWheelScroll.delta * 28.f, this->minScroll(), 0.f);
                return;
            }

            refreshButton.handleEvent(event, mouse);
            for (std::size_t idx : this->visibleIndices)
            {
                if (idx >= this->addButtons.size())
                    continue;
                // Ignore clicks landing on the fixed header strip.
                if (this->addButtons[idx].getBounds().top < HEADER_HEIGHT)
                    continue;
                this->addButtons[idx].handleEvent(event, mouse);
            }

            // A click on a row body (not on its Add button) folds/unfolds details.
            if (event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Left)
                this->toggleRowAt(mouse);
        }
    };
}

#endif
