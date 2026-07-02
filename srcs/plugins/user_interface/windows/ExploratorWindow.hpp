/*
 *  ╔════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════╗
 *  ║  ████████╗ █████╗ ████████╗ █████╗ ███╗   ██╗███████╗███████╗███████╗███████╗███████╗███████╗███████╗███████╗███████╗███████╗███████╗  ║
 *  ║     ██╔╝  ██╔══██╗╚══██╔══╝██╔══██╗████╗  ██║██╔════╝██╔════╝██╔════╝██╔════╝██╔════╝██╔════╝██╔════╝██╔════╝██╔════╝██╔════╝██╔════╝  ║
 *  ║     ██║   ███████║   ██║   ███████║██╔██╗ ██║█████╗  █████╗  █████╗  █████╗  █████╗  █████╗  █████╗  █████╗  █████╗  █████╗  █████╗    ║
 *  ║     ██║   ██╔══██║   ██║   ██╔══██║██║╚██╗██║██╔══╝  ██╔══╝  ██╔══╝  ██╔══╝  ██╔══╝  ██╔══╝  ██╔══╝  ██╔══╝  ██╔══╝  ██╔══╝  ██╔══╝    ║
 *  ║     ██║   ██║  ██║   ██║   ██║  ██║██║ ╚████║███████╗███████╗███████╗███████╗███████╗███████╗███████╗███████╗███████╗███████╗███████╗  ║
 *  ║     ╚═╝   ╚═╝  ╚═╝   ╚═╝   ╚═╝  ╚═╝╚═╝  ╚═══╝╚══════╝╚══════╝╚══════╝╚══════╝╚══════╝╚══════╝╚══════╝╚══════╝╚══════╝╚══════╝╚══════╝  ║
 *  ╚════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════╝
 *
 *  ┌────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────┐
 *  │ File   : ExploratorWindow.hpp
 *  │ Author : Tataneeeeeeeeeee
 *  │ Date   : 2026-05-13
 *  │ Brief  : Explorateur window de folie
 *  └────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────┘
*/

#ifndef EXPLORATOR_HPP
#define EXPLORATOR_HPP

#define SPACE_WIDTH 300.f

#include <algorithm>
#include <filesystem>
#include <vector>

#include "Window.hpp"

#include "../Theme.hpp"
#include "../LayoutPen.hpp"
#include "../components/Button.hpp"
#include "../components/TextField.hpp"
#include "../components/Separator.hpp"

namespace rc
{
    enum class ExploratorMode
    {
        OPEN,
        SAVE
    };

    struct ExploratorWindow : Window
    {
        ExploratorMode mode;
        std::string *resultPath = nullptr;

        struct ExplorerEntry
        {
            std::filesystem::path path;
            bool isDirectory = false;
        };

        std::filesystem::path currentPath;
        TextField currentPathField;
        std::vector<ExplorerEntry> entries;
        std::size_t selectedEntry = static_cast<std::size_t>(-1);

        float rowstartheight = 24.0f;
        float maxRowStartHeight = 24.0f;
        float minRowStartHeight = 24.0f;

        Button saveButton;
        TextField filenameField;

        Separator separator;

        void refreshEntries()
        {
            entries.clear();
            maxRowStartHeight = 24.f;
            selectedEntry = static_cast<std::size_t>(-1);

            try
            {
                if (currentPath.has_parent_path() && currentPath != currentPath.root_path())
                    entries.push_back({currentPath.parent_path(), true});

                std::vector<std::filesystem::directory_entry> directories;
                std::vector<std::filesystem::directory_entry> files;

                for (const auto &entry : std::filesystem::directory_iterator(currentPath))
                {
                    const std::string name = entry.path().filename().string();

                    if (!name.empty() && name[0] == '.')
                        continue;

                    if (entry.is_directory())
                        directories.push_back(entry);
                    else
                        files.push_back(entry);
                }

                auto sorter = [](const auto &lhs, const auto &rhs)
                {
                    return lhs.path().filename().string() < rhs.path().filename().string();
                };

                std::sort(directories.begin(), directories.end(), sorter);
                std::sort(files.begin(), files.end(), sorter);

                for (const auto &entry : directories)
                    entries.push_back({entry.path(), true});
                for (const auto &entry : files)
                    entries.push_back({entry.path(), false});
            }
            catch (const std::filesystem::filesystem_error &)
            {
            }
        }

        bool isInsideRow(const sf::FloatRect &bounds, const sf::Vector2i mouse) const
        {
            return bounds.contains(static_cast<float>(mouse.x), static_cast<float>(mouse.y));
        }

        void openDirectory(const std::filesystem::path &path)
        {
            currentPath = path;
            if (currentPath.empty())
                currentPath = std::filesystem::current_path();
            refreshEntries();
        }

        void create(sf::Font &f, ExploratorMode m, std::string &result)
        {
            font = &f;
            mode = m;
            this->resultPath = &result;

            currentPath = std::filesystem::current_path();

            windowWidth = 800;
            windowHeight = 600;
            windowTitle = (mode == ExploratorMode::SAVE) ? "Save scene" : "Load scene";

            running = true;
            thread = std::thread(&ExploratorWindow::loop, this);

            currentPathField.setFont(*font);
            currentPathField.setCharacterSize(14);
            currentPathField.box.setSize({620.f, 28.f});
            currentPathField.enabled = false;
            currentPathField.setValue(currentPath.string());

            filenameField.setFont(*font);
            filenameField.setCharacterSize(14);
            filenameField.box.setSize({240.f, 28.f});
            filenameField.setValue((mode == ExploratorMode::SAVE) ? "scene_save.cfg" : "");
            filenameField.enabled = (mode == ExploratorMode::SAVE);

            saveButton.setFont(*font);
            saveButton.setLabel((mode == ExploratorMode::SAVE) ? "Save" : "Open");
            saveButton.onClick = [&]
            {
                if (mode == ExploratorMode::SAVE)
                {
                    if (filenameField.value.empty())
                    {
                        *this->resultPath = currentPath.string() + "/scene_save.cfg";
                    }
                    else
                    {
                        *this->resultPath = currentPath.string() + "/" + filenameField.value;    
                    }
                }
                else
                {
                    if (selectedEntry != static_cast<std::size_t>(-1) && !entries[selectedEntry].isDirectory)
                    {
                        *this->resultPath = entries[selectedEntry].path.string();
                    }
                }
                destroy();
            };

            refreshEntries();
        }

        void drawUi() override
        {
            sf::RectangleShape panel;
            panel.setPosition({16.f, 16.f});
            panel.setSize({static_cast<float>(windowWidth) - 32.f, static_cast<float>(windowHeight) - 32.f});
            panel.setFillColor(theme::BG_PANEL);
            window.draw(panel);

            LayoutPen layout{28.f, rowstartheight, 10.f};

            drawTitle((mode == ExploratorMode::SAVE) ? "Save scene" : "Load scene", layout);
            layout.next(20.f);

            drawText("Current folder", layout);
            layout.next(18.f);

            currentPathField.layout(layout.x, layout.y, 720.f, 28.f);
            currentPathField.setValue(currentPath.string());
            window.draw(currentPathField);
            layout.next(34.f);

            separator.layout(layout.x, layout.y, 720.f);
            window.draw(separator);
            layout.next(14.f);

            drawText("Entries", layout);
            drawText(std::to_string(entries.size()) + " item(s)", {layout.x + 100.f, layout.y, layout.spacing});
            layout.next(18.f);

            const float rowHeight = 26.f;
            const float listWidth = 720.f;

            for (std::size_t index = 0; index < entries.size(); ++index)
            {
                const auto &entry = entries[index];

                sf::RectangleShape row;
                row.setPosition({layout.x, layout.y});
                row.setSize({listWidth, rowHeight});
                row.setFillColor(index % 2 == 0 ? theme::BG_ITEM : theme::BG_ITEM_ALT);

                if (selectedEntry == index)
                    row.setFillColor(theme::SELECTION_BG);

                window.draw(row);

                if (entry.isDirectory)
                {
                    sf::RectangleShape accent;
                    accent.setPosition({layout.x, layout.y});
                    accent.setSize({3.f, rowHeight});
                    accent.setFillColor(theme::ACCENT);
                    window.draw(accent);
                }

                sf::Text label;
                label.setFont(*font);
                label.setCharacterSize(13);
                label.setFillColor(theme::TEXT_MAIN);
                label.setPosition({layout.x + 10.f, layout.y + 4.f});

                std::string displayName = entry.path.filename().string();
                if (entry.path == currentPath.parent_path() && currentPath.has_parent_path())
                    displayName = "..";
                else if (entry.isDirectory)
                    displayName += "/";

                label.setString(displayName);
                window.draw(label);

                layout.next(rowHeight);
            }

            layout.next(8.f);
            separator.layout(layout.x, layout.y, 720.f);
            window.draw(separator);
            layout.next(14.f);

            if (mode == ExploratorMode::SAVE)
            {
                drawText("Filename", layout);
                layout.next(18.f);

                filenameField.layout(layout.x, layout.y, 320.f, 28.f);
                window.draw(filenameField);
                layout.next(38.f);

                saveButton.layout(layout.x, layout.y, 160.f, 34.f);
                window.draw(saveButton);
            }
            else
            {
                drawText("Select a folder or file", layout);
                layout.next(24.f);

                saveButton.layout(layout.x, layout.y, 160.f, 34.f);
                window.draw(saveButton);
            }

            if (maxRowStartHeight == minRowStartHeight)
                maxRowStartHeight = layout.y - rowstartheight;
        }

        void updateUi() override
        {
            sf::Vector2i mouse = sf::Mouse::getPosition(window);

            currentPathField.update(mouse);
            if (mode == ExploratorMode::SAVE)
                filenameField.update(mouse);

            saveButton.update(mouse);
        }

        void handleEvent(const sf::Event &event) override
        {
            sf::Vector2i mouse = sf::Mouse::getPosition(window);

            if (mode == ExploratorMode::SAVE)
                filenameField.handleEvent(event, mouse);

            if (event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Left)
            {
                LayoutPen layout{28.f, rowstartheight, 10.f};
                layout.next(20.f);
                layout.next(18.f);
                layout.next(34.f);
                layout.next(14.f);
                layout.next(18.f);

                const float rowHeight = 26.f;
                const float listWidth = 720.f;

                for (std::size_t index = 0; index < entries.size(); ++index)
                {
                    const sf::FloatRect rowBounds{layout.x, layout.y, listWidth, rowHeight};

                    if (isInsideRow(rowBounds, mouse))
                    {
                        selectedEntry = index;

                        if (entries[index].isDirectory)
                        {
                            openDirectory(entries[index].path);
                        }
                        else if (mode == ExploratorMode::SAVE)
                        {
                            filenameField.setValue(entries[index].path.filename().string());
                        }

                        return;
                    }

                    layout.next(rowHeight);
                }
            }

            if (event.type == sf::Event::MouseWheelScrolled)
            {
                if (event.mouseWheelScroll.wheel == sf::Mouse::VerticalWheel)
                {
                    const float availableHeight = windowHeight - 100.f;
                    
                    if (maxRowStartHeight >= availableHeight)
                    {
                        rowstartheight += event.mouseWheelScroll.delta * 10.f;
                        rowstartheight = std::min(rowstartheight, minRowStartHeight);
                        rowstartheight = std::max(rowstartheight, minRowStartHeight - maxRowStartHeight + availableHeight);
                    }
                    else
                    {
                        rowstartheight = minRowStartHeight;
                    }
                }
            }

            saveButton.handleEvent(event, mouse);
        }
    };
}

#endif