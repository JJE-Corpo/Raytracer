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

#ifndef MARKETWINDOW_HPP
#define MARKETWINDOW_HPP

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <mutex>
#include <thread>
#include <vector>
#include <SFML/Graphics.hpp>

#include "../../../common/Material.hpp"
#include "../../../common/MaterialLibrary.hpp"
#include "../Theme.hpp"
#include "../LayoutPen.hpp"
#include "../components/Button.hpp"
#include "Window.hpp"

namespace rc
{
    struct MarketWindow : Window
    {
        std::vector<Material> marketMaterials;
        std::vector<Button> addButtons;
        std::vector<bool> added;
        Button refreshButton;

        // "Add" clicks land here; DefaultScreen drains them on the main thread.
        std::mutex actionMutex;
        std::vector<Material> pendingAdds;

        float scrollY = 0.f;
        float contentHeight = 0.f;

        static constexpr float ROW_HEIGHT = 54.f;
        static constexpr float HEADER_HEIGHT = 66.f;
        static constexpr float ADD_BUTTON_WIDTH = 90.f;

        void create(sf::Font &f)
        {
            font = &f;
            windowWidth = 460;
            windowHeight = 520;
            windowTitle = "Material Market";

            refreshButton.setFont(*font);
            refreshButton.setLabel("Refresh");
            refreshButton.onClick = [this] { this->reload(); };

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
            this->scrollY = 0.f;
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

        static std::string fmt(float v)
        {
            char buf[32];
            std::snprintf(buf, sizeof(buf), "%.2f", v);
            return (std::string(buf));
        }

        static std::string materialProps(const Material &m)
        {
            if (m.model == MaterialModel::PBR)
                return ("metallic " + fmt(m.metallic) + "   roughness " + fmt(m.roughness) + "   ior " + fmt(m.ior));
            return ("shininess " + fmt(m.shininess) + "   refl " + fmt(m.reflectivity) + "   transp " + fmt(m.transparency));
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
            float y = HEADER_HEIGHT + 10.f + this->scrollY;

            for (std::size_t i = 0; i < this->marketMaterials.size(); ++i)
            {
                if (i < this->addButtons.size())
                    this->addButtons[i].layout(static_cast<float>(windowWidth) - 20.f - ADD_BUTTON_WIDTH, y + 8.f, ADD_BUTTON_WIDTH, 30.f);
                y += ROW_HEIGHT;
            }
            this->contentHeight = y - (HEADER_HEIGHT + 10.f + this->scrollY);
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

            for (std::size_t i = 0; i < this->marketMaterials.size(); ++i)
            {
                const Material &m = this->marketMaterials[i];
                const Color c = m.getBaseColor().toColor();

                drawRect(x0, y + 6.f, 24.f, 24.f, sf::Color(c.r, c.g, c.b), 1.f, theme::OUTLINE_MID);
                drawColoredText(m.getName(), x0 + 36.f, y + 4.f, 15, theme::TEXT_MAIN);

                std::string model = m.getModelName();
                std::transform(model.begin(), model.end(), model.begin(), ::toupper);
                drawColoredText(model, x0 + 36.f, y + 26.f, 11, theme::ACCENT);
                drawColoredText(materialProps(m), x0 + 90.f, y + 26.f, 11, theme::TEXT_DIM);

                if (i < this->addButtons.size())
                    window.draw(this->addButtons[i]);
                y += ROW_HEIGHT;
            }

            // Opaque header drawn last so scrolled rows slide underneath it.
            drawRect(0.f, 0.f, static_cast<float>(windowWidth), 64.f, theme::BG_BAR);
            drawRect(0.f, 64.f, static_cast<float>(windowWidth), 2.f, theme::ACCENT);
            drawColoredText("Material Market", 20.f, 14.f, 22, theme::TEXT_WHITE);
            drawColoredText("~/.raytracer/materials", 20.f, 42.f, 12, theme::TEXT_DIM);

            refreshButton.layout(static_cast<float>(windowWidth) - 20.f - ADD_BUTTON_WIDTH, 18.f, ADD_BUTTON_WIDTH, 30.f);
            window.draw(refreshButton);
        }

        void updateUi() override
        {
            sf::Vector2i mouse = sf::Mouse::getPosition(window);

            refreshButton.update(mouse);
            for (std::size_t i = 0; i < this->addButtons.size(); ++i)
            {
                // Rows scrolled up behind the header must not react to hover.
                if (this->addButtons[i].getBounds().top < HEADER_HEIGHT)
                    this->addButtons[i].hovered = false;
                else
                    this->addButtons[i].update(mouse);
            }
        }

        void handleEvent(const sf::Event &event) override
        {
            sf::Vector2i mouse = sf::Mouse::getPosition(window);

            if (event.type == sf::Event::MouseWheelScrolled)
            {
                this->scrollY = std::clamp(this->scrollY + event.mouseWheelScroll.delta * 28.f, this->minScroll(), 0.f);
                return;
            }

            refreshButton.handleEvent(event, mouse);
            for (std::size_t i = 0; i < this->addButtons.size(); ++i)
            {
                // Ignore clicks landing on the fixed header strip.
                if (this->addButtons[i].getBounds().top < HEADER_HEIGHT)
                    continue;
                this->addButtons[i].handleEvent(event, mouse);
            }
        }
    };
}

#endif
