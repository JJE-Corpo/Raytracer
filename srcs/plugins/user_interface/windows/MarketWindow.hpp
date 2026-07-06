#ifndef MARKETWINDOW_HPP
#define MARKETWINDOW_HPP

#include <algorithm>
#include <cctype>
#include <cstddef>
#include <cstdio>
#include <map>
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
#include "OnlineMarket.hpp"
#include "Window.hpp"

namespace rc
{
    struct MarketWindow : Window
    {
        enum class Tab { Local, Online };

        Tab activeTab = Tab::Local;

        std::vector<Material> marketMaterials;
        std::vector<Button> addButtons;
        std::vector<bool> added;
        Button refreshButton;

        TextField searchField;
        std::vector<std::size_t> visibleIndices;
        std::vector<float> rowTops;

        int expandedIndex = -1;

        std::mutex actionMutex;
        std::vector<Material> pendingAdds;

        OnlineMarket onlineMarket;
        std::map<std::string, sf::Texture> onlineTex;
        std::vector<std::size_t> onlineVisible;
        std::vector<float> onlineRowTops;
        int expandedOnline = -1;

        float scrollY = 0.f;
        float contentHeight = 0.f;

        static constexpr float ROW_HEIGHT = 54.f;
        static constexpr float HEADER_HEIGHT = 124.f;
        static constexpr float ADD_BUTTON_WIDTH = 90.f;
        static constexpr float DETAIL_LINE_H = 18.f;
        static constexpr float DETAIL_PAD_TOP = 8.f;
        static constexpr float DETAIL_PAD_BOTTOM = 12.f;
        static constexpr float ONLINE_ROW_HEIGHT = 60.f;
        static constexpr float THUMB_SIZE = 44.f;
        static constexpr float TAB_Y = 48.f;
        static constexpr float TAB_H = 30.f;
        static constexpr float TAB_W = 130.f;
        static constexpr float TAB_LOCAL_X = 20.f;
        static constexpr float TAB_ONLINE_X = 156.f;

        ~MarketWindow() override
        {
            this->destroy();
            this->onlineMarket.stop();
        }

        void create(sf::Font &f)
        {
            font = &f;
            windowWidth = 460;
            windowHeight = 520;
            windowTitle = "Material Market";

            refreshButton.setFont(*font);
            refreshButton.setLabel("Refresh");
            refreshButton.onClick = [this] {
                if (this->activeTab == Tab::Local)
                    this->reload();
                else
                {
                    this->onlineTex.clear();
                    this->expandedOnline = -1;
                    this->onlineMarket.requestFetch();
                }
            };

            searchField.setFont(*font);
            searchField.setCharacterSize(14);

            this->reload();

            running = true;
            thread = std::thread(&MarketWindow::loop, this);
        }

        void drainPendingAdds(std::vector<Material> &out)
        {
            std::lock_guard<std::mutex> lock(this->actionMutex);
            if (this->pendingAdds.empty())
                return;
            out.insert(out.end(), this->pendingAdds.begin(), this->pendingAdds.end());
            this->pendingAdds.clear();
        }

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

        // Clicking "Add" on an online entry kicks off a background download of the
        // full-res diffuse texture. The material is only queued into the scene once
        // the download settles (see finalizeOnlineAdds), so the UI never blocks.
        void queueAddOnline(std::size_t index)
        {
            std::lock_guard<std::mutex> lock(this->onlineMarket.mutex);
            if (index >= this->onlineMarket.materials.size())
                return;
            OnlineMaterial &om = this->onlineMarket.materials[index];
            if (om.added)
                return;
            om.added = true;
            om.texState = OnlineMaterial::Tex::Queued;
        }

        // Turn any online entry whose texture download has settled into a scene
        // material (with the texture wired up when the download succeeded, or a
        // flat average-colour fallback when it failed). Runs on the UI thread.
        void finalizeOnlineAdds()
        {
            std::vector<Material> ready;
            {
                std::lock_guard<std::mutex> lock(this->onlineMarket.mutex);
                for (OnlineMaterial &om : this->onlineMarket.materials)
                {
                    if (!om.added || om.queuedToScene)
                        continue;
                    if (om.texState != OnlineMaterial::Tex::Ready && om.texState != OnlineMaterial::Tex::Failed)
                        continue;

                    Material mat;
                    mat.model = MaterialModel::PBR;
                    mat.name = om.name;
                    mat.baseColor = om.hasAvgColor ? om.avgColor : ColorF{0.6f, 0.6f, 0.6f};
                    mat.roughness = 0.6f;
                    mat.metallic = 0.0f;
                    if (om.texState == OnlineMaterial::Tex::Ready && !om.texturePath.empty())
                    {
                        mat.texture_map = om.texturePath;
                        mat.texture_map_enabled = true;
                    }
                    om.queuedToScene = true;
                    ready.push_back(mat);
                }
            }

            if (ready.empty())
                return;
            {
                std::lock_guard<std::mutex> lock(this->actionMutex);
                this->pendingAdds.insert(this->pendingAdds.end(), ready.begin(), ready.end());
            }
            for (const Material &mat : ready)
                MaterialLibrary::save(mat);
        }

        static std::string toLower(const std::string &s)
        {
            std::string out = s;
            std::transform(out.begin(), out.end(), out.begin(), [](unsigned char c) {
                return static_cast<char>(std::tolower(c));
            });
            return (out);
        }

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

        void drawTab(const std::string &label, float x, bool active)
        {
            drawRect(x, TAB_Y, TAB_W, TAB_H, active ? theme::BG_ITEM : theme::BG_PANEL);
            if (active)
                drawRect(x, TAB_Y + TAB_H - 3.f, TAB_W, 3.f, theme::ACCENT);
            drawColoredText(label, x + 14.f, TAB_Y + 6.f, 14, active ? theme::TEXT_WHITE : theme::TEXT_DIM);
        }

        int tabAt(const sf::Vector2i &mouse) const
        {
            if (mouse.y < TAB_Y || mouse.y >= TAB_Y + TAB_H)
                return (-1);
            if (mouse.x >= TAB_LOCAL_X && mouse.x < TAB_LOCAL_X + TAB_W)
                return (0);
            if (mouse.x >= TAB_ONLINE_X && mouse.x < TAB_ONLINE_X + TAB_W)
                return (1);
            return (-1);
        }

        void setActiveTab(Tab tab)
        {
            if (this->activeTab == tab)
                return;
            this->activeTab = tab;
            this->scrollY = 0.f;
            if (tab == Tab::Online)
                this->onlineMarket.requestFetch();
        }

        void drawHeader()
        {
            drawRect(0.f, 0.f, static_cast<float>(windowWidth), HEADER_HEIGHT - 2.f, theme::BG_BAR);
            drawRect(0.f, HEADER_HEIGHT - 2.f, static_cast<float>(windowWidth), 2.f, theme::ACCENT);
            drawColoredText("Material Market", 20.f, 12.f, 20, theme::TEXT_WHITE);

            refreshButton.layout(static_cast<float>(windowWidth) - 20.f - ADD_BUTTON_WIDTH, 12.f, ADD_BUTTON_WIDTH, 28.f);
            window.draw(refreshButton);

            drawTab("Local (" + std::to_string(this->marketMaterials.size()) + ")", TAB_LOCAL_X, this->activeTab == Tab::Local);
            drawTab("Online", TAB_ONLINE_X, this->activeTab == Tab::Online);

            searchField.layout(20.f, 86.f, static_cast<float>(windowWidth) - 40.f, 28.f);
            window.draw(searchField);
            if (searchField.value.empty() && !searchField.focused)
                drawColoredText("Search materials...", 30.f, 91.f, 13, theme::TEXT_DIM);
        }

        void drawLocal()
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
        }

        static bool onlineMatches(const OnlineMaterial &m, const std::string &query)
        {
            return (toLower(m.name).find(query) != std::string::npos
                || toLower(m.id).find(query) != std::string::npos
                || toLower(m.categoriesText).find(query) != std::string::npos
                || toLower(m.tagsText).find(query) != std::string::npos);
        }

        static std::vector<std::pair<std::string, std::string>> onlineDetailLines(const OnlineMaterial &m)
        {
            return {
                {"Id", m.id},
                {"Author", m.author.empty() ? "-" : m.author},
                {"Categories", m.categoriesText.empty() ? "-" : m.categoriesText},
                {"Tags", m.tagsText.empty() ? "-" : m.tagsText},
                {"Max res", m.resolutionX > 0 ? (std::to_string(m.resolutionX) + " x " + std::to_string(m.resolutionY)) : "-"},
                {"Source", "Poly Haven (CC0)"},
            };
        }

        static float onlineDetailHeight(const OnlineMaterial &m)
        {
            return (static_cast<float>(onlineDetailLines(m).size()) * DETAIL_LINE_H + DETAIL_PAD_TOP + DETAIL_PAD_BOTTOM);
        }

        float onlineRowHeight(std::size_t idx, const OnlineMaterial &m) const
        {
            float h = ONLINE_ROW_HEIGHT;
            if (static_cast<int>(idx) == this->expandedOnline)
                h += onlineDetailHeight(m);
            return (h);
        }

        void drawOnlineRow(OnlineMaterial &m, std::size_t idx, float y)
        {
            const bool expanded = static_cast<int>(idx) == this->expandedOnline;

            if (expanded)
                drawRect(0.f, y, static_cast<float>(windowWidth), ONLINE_ROW_HEIGHT, theme::BG_ITEM);

            if (m.thumb == OnlineMaterial::Thumb::None)
                m.thumb = OnlineMaterial::Thumb::Queued;
            if (m.thumb == OnlineMaterial::Thumb::ImageReady)
            {
                sf::Texture &tex = this->onlineTex[m.id];
                tex.setSmooth(true);
                tex.loadFromImage(m.image);
                m.image = sf::Image();
                m.thumb = OnlineMaterial::Thumb::Ready;
            }

            const float tx = 20.f;
            const float ty = y + 8.f;
            auto texIt = this->onlineTex.find(m.id);
            if (m.thumb == OnlineMaterial::Thumb::Ready && texIt != this->onlineTex.end())
            {
                const sf::Vector2u sz = texIt->second.getSize();
                sf::Sprite sprite(texIt->second);
                if (sz.x > 0 && sz.y > 0)
                    sprite.setScale(THUMB_SIZE / static_cast<float>(sz.x), THUMB_SIZE / static_cast<float>(sz.y));
                sprite.setPosition({tx, ty});
                window.draw(sprite);
            }
            else
            {
                drawRect(tx, ty, THUMB_SIZE, THUMB_SIZE, theme::BG_CONTROL, 1.f, theme::OUTLINE_MID);
                if (m.thumb == OnlineMaterial::Thumb::Failed)
                    drawColoredText("x", tx + 18.f, ty + 12.f, 16, theme::TEXT_DIM);
                else
                    drawColoredText("...", tx + 12.f, ty + 12.f, 16, theme::TEXT_DIM);
            }

            const float textX = tx + THUMB_SIZE + 12.f;
            drawColoredText(m.name, textX, y + 6.f, 15, theme::TEXT_MAIN);
            if (!m.categoriesText.empty())
                drawColoredText(m.categoriesText, textX, y + 28.f, 11, theme::TEXT_DIM);
            if (!m.author.empty())
                drawColoredText("by " + m.author, textX, y + 42.f, 11, theme::ACCENT);

            const float bx = static_cast<float>(windowWidth) - 20.f - ADD_BUTTON_WIDTH;
            const bool downloading = m.texState == OnlineMaterial::Tex::Queued
                || m.texState == OnlineMaterial::Tex::Downloading;
            const char *label = !m.added ? "Add" : (downloading ? "..." : "Added");
            drawRect(bx, y + 8.f, ADD_BUTTON_WIDTH, 30.f,
                m.added ? theme::BG_CONTROL_HOVER : theme::BG_ITEM, 1.f, theme::OUTLINE_MID);
            drawColoredText(label, bx + 30.f, y + 15.f, 14, theme::TEXT_WHITE);

            if (expanded)
            {
                const auto lines = onlineDetailLines(m);
                const float dy = y + ONLINE_ROW_HEIGHT;
                const float width = static_cast<float>(windowWidth) - 24.f;
                drawRect(12.f, dy, width, onlineDetailHeight(m), theme::BG_PANEL);
                drawRect(12.f, dy, 3.f, onlineDetailHeight(m), theme::ACCENT);
                float ly = dy + DETAIL_PAD_TOP;
                for (const auto &line : lines)
                {
                    drawColoredText(line.first, 26.f, ly, 12, theme::TEXT_DIM);
                    drawColoredText(line.second, 130.f, ly, 12, theme::TEXT_MAIN);
                    ly += DETAIL_LINE_H;
                }
            }
        }

        void drawOnline()
        {
            const float x0 = 20.f;
            const OnlineMarket::Status st = this->onlineMarket.status.load();

            std::lock_guard<std::mutex> lock(this->onlineMarket.mutex);
            std::vector<OnlineMaterial> &mats = this->onlineMarket.materials;

            const std::string query = toLower(this->searchField.value);
            this->onlineVisible.clear();
            for (std::size_t i = 0; i < mats.size(); ++i)
                if (query.empty() || onlineMatches(mats[i], query))
                    this->onlineVisible.push_back(i);

            const float start = HEADER_HEIGHT + 10.f + this->scrollY;
            float y = start;

            if (st == OnlineMarket::Status::Idle || st == OnlineMarket::Status::Loading)
            {
                drawColoredText("Loading Poly Haven catalogue...", x0, y + 4.f, 14, theme::TEXT_DIM);
                this->contentHeight = 0.f;
                return;
            }
            if (st == OnlineMarket::Status::Error)
            {
                drawColoredText("Could not load online materials.", x0, y + 4.f, 14, theme::TEXT_MAIN);
                drawColoredText(this->onlineMarket.errorText, x0, y + 28.f, 12, theme::TEXT_DIM);
                this->contentHeight = 0.f;
                return;
            }
            if (mats.empty())
            {
                drawColoredText("No online materials found.", x0, y + 4.f, 14, theme::TEXT_DIM);
                this->contentHeight = 0.f;
                return;
            }
            if (this->onlineVisible.empty())
            {
                drawColoredText("No material matches \"" + searchField.value + "\".", x0, y + 4.f, 14, theme::TEXT_DIM);
                this->contentHeight = 0.f;
                return;
            }

            this->onlineRowTops.clear();
            for (std::size_t k = 0; k < this->onlineVisible.size(); ++k)
            {
                const std::size_t idx = this->onlineVisible[k];
                const float rowH = this->onlineRowHeight(idx, mats[idx]);
                this->onlineRowTops.push_back(y);
                const bool onScreen = (y + rowH > HEADER_HEIGHT) && (y < static_cast<float>(windowHeight));
                if (onScreen)
                    this->drawOnlineRow(mats[idx], idx, y);
                y += rowH;
            }
            this->contentHeight = y - start;
        }

        void drawUi() override
        {
            if (this->activeTab == Tab::Local)
                this->drawLocal();
            else
                this->drawOnline();

            this->drawHeader();
        }

        void updateUi() override
        {
            sf::Vector2i mouse = sf::Mouse::getPosition(window);

            // Hand finished texture downloads to the scene, whatever tab is open.
            this->finalizeOnlineAdds();

            searchField.update(mouse);
            refreshButton.update(mouse);
            if (this->activeTab != Tab::Local)
                return;
            for (std::size_t idx : this->visibleIndices)
            {
                if (idx >= this->addButtons.size())
                    continue;
                if (this->addButtons[idx].getBounds().top < HEADER_HEIGHT)
                    this->addButtons[idx].hovered = false;
                else
                    this->addButtons[idx].update(mouse);
            }
        }

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

        void handleOnlineClick(const sf::Vector2i &mouse)
        {
            if (mouse.y < HEADER_HEIGHT)
                return;
            const float buttonLeft = static_cast<float>(windowWidth) - 20.f - ADD_BUTTON_WIDTH;

            for (std::size_t k = 0; k < this->onlineVisible.size() && k < this->onlineRowTops.size(); ++k)
            {
                const float top = this->onlineRowTops[k];
                if (mouse.y < top || mouse.y >= top + ONLINE_ROW_HEIGHT)
                    continue;
                const std::size_t idx = this->onlineVisible[k];
                if (mouse.x >= buttonLeft && mouse.x < buttonLeft + ADD_BUTTON_WIDTH
                    && mouse.y >= top + 8.f && mouse.y < top + 38.f)
                    this->queueAddOnline(idx);
                else if (mouse.x >= 20.f && mouse.x < buttonLeft)
                    this->expandedOnline = (this->expandedOnline == static_cast<int>(idx)) ? -1 : static_cast<int>(idx);
                return;
            }
        }

        void handleEvent(const sf::Event &event) override
        {
            sf::Vector2i mouse = sf::Mouse::getPosition(window);

            searchField.handleEvent(event, mouse);

            if (event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Left)
            {
                const int tab = this->tabAt(mouse);
                if (tab == 0)
                {
                    this->setActiveTab(Tab::Local);
                    return;
                }
                if (tab == 1)
                {
                    this->setActiveTab(Tab::Online);
                    return;
                }
            }

            if (event.type == sf::Event::MouseWheelScrolled)
            {
                this->scrollY = std::clamp(this->scrollY + event.mouseWheelScroll.delta * 28.f, this->minScroll(), 0.f);
                return;
            }

            refreshButton.handleEvent(event, mouse);

            if (this->activeTab == Tab::Online)
            {
                if (event.type == sf::Event::MouseButtonReleased && event.mouseButton.button == sf::Mouse::Left)
                    this->handleOnlineClick(mouse);
                return;
            }

            for (std::size_t idx : this->visibleIndices)
            {
                if (idx >= this->addButtons.size())
                    continue;
                if (this->addButtons[idx].getBounds().top < HEADER_HEIGHT)
                    continue;
                this->addButtons[idx].handleEvent(event, mouse);
            }

            if (event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Left)
                this->toggleRowAt(mouse);
        }
    };
}

#endif
