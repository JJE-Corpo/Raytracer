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
#include <map>
#include <ctime>
#include <iomanip>
#include <chrono>

#include "Window.hpp"

#include "../Theme.hpp"
#include "../VerticalLayout.hpp"
#include "../components/Button.hpp"
#include "../components/TextField.hpp"
#include "../components/Separator.hpp"

#include "../../../utils/TimestampUtils.h"

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
            std::filesystem::file_time_type lastModified;
            uintmax_t size = 0;
            bool isDirectory = false;
        };

        int currentEntryIndex = -1;
        std::vector<std::filesystem::path> pathHistory;
        TextField currentPathField;
        std::vector<ExplorerEntry> entries;
        std::vector<std::string> selectedEntries;
        std::size_t selectedEntry = static_cast<std::size_t>(-1);

        float rowstartheight = 24.0f;
        float maxRowStartHeight = 24.0f;
        float minRowStartHeight = 24.0f;

        Button nameSortButton;
        Button sizeSortButton;
        Button modifiedSortButton;

        Button backButton;
        Button nextButton;
        Button upButton;

        Button saveButton;
        TextField filenameField;

        Separator separator;

        std::filesystem::path getCurrentPath()
        {
            return pathHistory.empty() ? std::filesystem::current_path() : pathHistory[currentEntryIndex];
        }

        void refreshEntries()
        {
            entries.clear();
            maxRowStartHeight = 24.f;
            selectedEntry = static_cast<std::size_t>(-1);

            try
            {
                // if (getCurrentPath().has_parent_path() && getCurrentPath() != getCurrentPath().root_path())
                //     entries.push_back({getCurrentPath().parent_path(), true});

                std::vector<std::filesystem::directory_entry> directories;
                std::vector<std::filesystem::directory_entry> files;

                for (const auto &entry : std::filesystem::directory_iterator(getCurrentPath()))
                {
                    const std::string name = entry.path().filename().string();

                    if (!name.empty() && name[0] == '.')
                        continue;

                    if (entry.is_directory())
                        directories.push_back(entry);
                    else
                    {
                        if (std::find(selectedEntries.begin(), selectedEntries.end(), entry.path().extension().string()) != selectedEntries.end())
                            files.push_back(entry);
                    }
                }

                auto sorter = [](const auto &lhs, const auto &rhs)
                {
                    return lhs.path().filename().string() < rhs.path().filename().string();
                };

                std::sort(directories.begin(), directories.end(), sorter);
                std::sort(files.begin(), files.end(), sorter);

                for (const auto &entry : directories)
                {
                    uintmax_t size = 0;
                    
                    for (const auto &_ : std::filesystem::recursive_directory_iterator(entry.path()))
                        size++;

                    entries.push_back({entry.path(), entry.last_write_time(), size, true});
                }
                for (const auto &entry : files)
                    entries.push_back({entry.path(), entry.last_write_time(), entry.file_size(), false});
            }
            catch (const std::filesystem::filesystem_error &)
            {
            }
        }

        void setSelectedEntry(std::vector<std::string> selected)
        {
            this->selectedEntries = selected;
        }

        bool isInsideRow(const sf::FloatRect &bounds, const sf::Vector2i mouse) const
        {
            return bounds.contains(static_cast<float>(mouse.x), static_cast<float>(mouse.y));
        }

        void openDirectory(const std::filesystem::path &path)
        {
            if (currentEntryIndex >= 0 && currentEntryIndex + 1 < static_cast<int>(pathHistory.size()))
                pathHistory.erase(pathHistory.begin() + currentEntryIndex + 1, pathHistory.end());

            pathHistory.push_back(path);
            currentEntryIndex = static_cast<int>(pathHistory.size()) - 1;

            refreshEntries();
        }

        void goBack()
        {
            if (currentEntryIndex > 0)
            {
                currentEntryIndex -= 1;
                refreshEntries();
            }
        }

        void goForward()
        {
            if (currentEntryIndex < static_cast<int>(pathHistory.size()) - 1)
            {
                currentEntryIndex += 1;
                refreshEntries();
            }
        }

        void goUp()
        {
            if (getCurrentPath().has_parent_path())
            {
                openDirectory(getCurrentPath().parent_path());
            }
        }

        void create(sf::Font &f, ExploratorMode m, std::string &result)
        {
            font = &f;
            mode = m;
            this->resultPath = &result;

            currentEntryIndex = 0;
            
            pathHistory.clear();
            pathHistory.push_back(std::filesystem::current_path());

            windowWidth = 1000;
            windowHeight = 600;
            windowTitle = (mode == ExploratorMode::SAVE) ? "Save scene" : "Load scene";

            running = true;
            thread = std::thread(&ExploratorWindow::loop, this);

            currentPathField.setFont(*font);
            currentPathField.setCharacterSize(14);
            currentPathField.box.setSize({620.f, 28.f});
            currentPathField.enabled = true;
            currentPathField.setValue(getCurrentPath().string());
            currentPathField.onValidate = [&](const std::string &v) -> bool
            {
                std::error_code ec;
                std::filesystem::path typed(v);

                if (std::filesystem::is_directory(typed, ec))
                    openDirectory(typed);
                else
                    currentPathField.setValue(getCurrentPath().string());

                return true;
            };

            nameSortButton.setFont(*font);
            nameSortButton.setLabel("Name");
            nameSortButton.onClick = [&]
            {
                std::sort(entries.begin(), entries.end(), [](const auto &lhs, const auto &rhs)
                {
                    return lhs.path.filename().string() < rhs.path.filename().string();
                });
            };

            sizeSortButton.setFont(*font);
            sizeSortButton.setLabel("Size");
            sizeSortButton.onClick = [&]
            {
                std::sort(entries.begin(), entries.end(), [](const auto &lhs, const auto &rhs)
                {
                    if (lhs.isDirectory && !rhs.isDirectory)
                        return true;
                    if (!lhs.isDirectory && rhs.isDirectory)
                        return false;
                    return lhs.size < rhs.size;
                });
            };

            modifiedSortButton.setFont(*font);
            modifiedSortButton.setLabel("Modified");
            modifiedSortButton.onClick = [&]
            {
                std::sort(entries.begin(), entries.end(), [](const auto &lhs, const auto &rhs)
                {
                    return std::filesystem::last_write_time(lhs.path) < std::filesystem::last_write_time(rhs.path);
                });
            };

            backButton.setFont(*font);
            backButton.setLabel("<");
            backButton.onClick = [&] { goBack(); };

            nextButton.setFont(*font);
            nextButton.setLabel(">");
            nextButton.onClick = [&] { goForward(); };

            upButton.setFont(*font);
            upButton.setLabel("^");
            upButton.onClick = [&] { goUp(); };

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
                        *this->resultPath = getCurrentPath().string() + "/scene_save.cfg";
                    }
                    else
                    {
                        *this->resultPath = getCurrentPath().string() + "/" + filenameField.value;    
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

            VerticalLayout layout{28.f, rowstartheight, 10.f};

            backButton.layout(layout.x, layout.y, 34.f, 28.f);
            window.draw(backButton);

            nextButton.layout(layout.x + 40.f, layout.y, 34.f, 28.f);
            window.draw(nextButton);

            currentPathField.layout(layout.x + 80.f, layout.y, 825.f, 28.f);
            if (!currentPathField.focused)
                currentPathField.setValue(getCurrentPath().string());
            window.draw(currentPathField);

            upButton.layout(layout.x + 910.f, layout.y, 34.f, 28.f);
            window.draw(upButton);
            layout.next(34.f);

            nameSortButton.layout(layout.x, layout.y, 683.f, 28.f);
            window.draw(nameSortButton);

            sizeSortButton.layout(layout.x + 693.f, layout.y, 120.f, 28.f);
            window.draw(sizeSortButton);

            modifiedSortButton.layout(layout.x + 823.f, layout.y, 120.f, 28.f);
            window.draw(modifiedSortButton);
            layout.next(25.f);

            separator.layout(layout.x, layout.y, 943.f);
            window.draw(separator);
            layout.next(5.f);

            const float rowHeight = 26.f;
            const float listWidth = 943.f;

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
                if (entry.path == getCurrentPath().parent_path() && getCurrentPath().has_parent_path())
                    displayName = "..";
                else if (entry.isDirectory)
                    displayName += "/";

                label.setString(displayName);
                window.draw(label);

                label.setPosition({layout.x + 713.f, layout.y + 4.f});
                label.setString(entry.isDirectory ? std::to_string(entry.size) + " items" : std::to_string(entry.size) + " bytes");
                window.draw(label);

                label.setPosition({layout.x + 843.f, layout.y + 4.f});
                label.setString(TimestampUtils::toString(entry.lastModified, "%d %b %Y"));
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

            nameSortButton.update(mouse);
            sizeSortButton.update(mouse);
            modifiedSortButton.update(mouse);

            backButton.update(mouse);
            nextButton.update(mouse);
            upButton.update(mouse);

            saveButton.update(mouse);
        }

        void handleEvent(const sf::Event &event) override
        {
            sf::Vector2i mouse = sf::Mouse::getPosition(window);

            if (mode == ExploratorMode::SAVE)
                filenameField.handleEvent(event, mouse);

            currentPathField.handleEvent(event, mouse);

            if (event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Left)
            {
                VerticalLayout layout{28.f, rowstartheight, 10.f};
                layout.next(34.f);
                layout.next(5.f);
                layout.next(25.f);

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

            nameSortButton.handleEvent(event, mouse);
            sizeSortButton.handleEvent(event, mouse);
            modifiedSortButton.handleEvent(event, mouse);

            backButton.handleEvent(event, mouse);
            nextButton.handleEvent(event, mouse);
            upButton.handleEvent(event, mouse);

            saveButton.handleEvent(event, mouse);
        }
    };
}

#endif