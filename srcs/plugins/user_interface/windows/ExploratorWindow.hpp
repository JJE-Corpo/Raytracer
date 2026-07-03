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
#include "../LayoutPen.hpp"
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

        enum class SortColumn
        {
            NAME,
            SIZE,
            MODIFIED
        };

        int currentEntryIndex = -1;
        std::vector<std::filesystem::path> pathHistory;
        TextField currentPathField;
        std::vector<ExplorerEntry> entries;
        std::vector<std::string> selectedEntries;
        std::size_t selectedEntry = static_cast<std::size_t>(-1);

        SortColumn sortColumn = SortColumn::NAME;
        bool sortAscending = true;

        static constexpr float LEFT_X = 28.f;
        static constexpr float LIST_TOP = 118.f;
        static constexpr float ROW_HEIGHT = 26.f;
        static constexpr float ROW_PITCH = 36.f;
        static constexpr float LIST_WIDTH = 943.f;

        float scrollOffset = 0.f;
        float maxScroll = 0.f;

        float listBottom() const
        {
            return static_cast<float>(windowHeight) - 90.f;
        }

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
            scrollOffset = 0.f;
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
                    std::error_code ec;

                    for (auto it = std::filesystem::directory_iterator(entry.path(), ec);
                         !ec && it != std::filesystem::directory_iterator();
                         it.increment(ec))
                        size++;

                    entries.push_back({entry.path(), entry.last_write_time(), size, true});
                }
                for (const auto &entry : files)
                    entries.push_back({entry.path(), entry.last_write_time(), entry.file_size(), false});
            }
            catch (const std::filesystem::filesystem_error &)
            {
            }

            applySort();
        }

        void drawTriangle(float cx, float cy, bool pointingUp, float halfWidth = 5.f, float halfHeight = 3.5f)
        {
            sf::ConvexShape triangle;
            triangle.setPointCount(3);
            triangle.setFillColor(theme::TEXT_WHITE);

            if (pointingUp)
            {
                triangle.setPoint(0, {cx, cy - halfHeight});
                triangle.setPoint(1, {cx - halfWidth, cy + halfHeight});
                triangle.setPoint(2, {cx + halfWidth, cy + halfHeight});
            }
            else
            {
                triangle.setPoint(0, {cx - halfWidth, cy - halfHeight});
                triangle.setPoint(1, {cx + halfWidth, cy - halfHeight});
                triangle.setPoint(2, {cx, cy + halfHeight});
            }

            window.draw(triangle);
        }

        void drawSortArrow(float buttonX, float buttonWidth, float buttonY)
        {
            drawTriangle(buttonX + buttonWidth - 16.f, buttonY + 14.f, sortAscending);
        }

        void applySort()
        {
            switch (sortColumn)
            {
            case SortColumn::NAME:
                std::sort(entries.begin(), entries.end(), [this](const auto &lhs, const auto &rhs)
                {
                    const std::string l = lhs.path.filename().string();
                    const std::string r = rhs.path.filename().string();
                    return sortAscending ? l < r : l > r;
                });
                break;
            case SortColumn::SIZE:
                std::sort(entries.begin(), entries.end(), [this](const auto &lhs, const auto &rhs)
                {
                    if (lhs.isDirectory != rhs.isDirectory)
                        return lhs.isDirectory;
                    return sortAscending ? lhs.size < rhs.size : lhs.size > rhs.size;
                });
                break;
            case SortColumn::MODIFIED:
                std::sort(entries.begin(), entries.end(), [this](const auto &lhs, const auto &rhs)
                {
                    return sortAscending ? lhs.lastModified < rhs.lastModified : lhs.lastModified > rhs.lastModified;
                });
                break;
            }
        }

        void sortBy(SortColumn column)
        {
            if (sortColumn == column)
                sortAscending = !sortAscending;
            else
            {
                sortColumn = column;
                sortAscending = true;
            }

            applySort();
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
            nameSortButton.onClick = [&] { sortBy(SortColumn::NAME); };

            sizeSortButton.setFont(*font);
            sizeSortButton.setLabel("Size");
            sizeSortButton.onClick = [&] { sortBy(SortColumn::SIZE); };

            modifiedSortButton.setFont(*font);
            modifiedSortButton.setLabel("Modified");
            modifiedSortButton.onClick = [&] { sortBy(SortColumn::MODIFIED); };

            backButton.setFont(*font);
            backButton.setLabel("<");
            backButton.onClick = [&] { goBack(); };

            nextButton.setFont(*font);
            nextButton.setLabel(">");
            nextButton.onClick = [&] { goForward(); };

            upButton.setFont(*font);
            upButton.setLabel("");
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

            const float visibleBottom = listBottom();
            const float visibleHeight = visibleBottom - LIST_TOP;
            const float contentHeight = static_cast<float>(entries.size()) * ROW_PITCH;

            maxScroll = std::max(0.f, contentHeight - visibleHeight);
            scrollOffset = std::max(-maxScroll, std::min(0.f, scrollOffset));

            for (std::size_t index = 0; index < entries.size(); ++index)
            {
                const float rowY = LIST_TOP + scrollOffset + static_cast<float>(index) * ROW_PITCH;

                if (rowY + ROW_HEIGHT < LIST_TOP || rowY > visibleBottom)
                    continue;

                const auto &entry = entries[index];

                sf::RectangleShape row;
                row.setPosition({LEFT_X, rowY});
                row.setSize({LIST_WIDTH, ROW_HEIGHT});
                row.setFillColor(index % 2 == 0 ? theme::BG_ITEM : theme::BG_ITEM_ALT);

                if (selectedEntry == index)
                    row.setFillColor(theme::SELECTION_BG);

                window.draw(row);

                if (entry.isDirectory)
                {
                    sf::RectangleShape accent;
                    accent.setPosition({LEFT_X, rowY});
                    accent.setSize({3.f, ROW_HEIGHT});
                    accent.setFillColor(theme::ACCENT);
                    window.draw(accent);
                }

                sf::Text label;
                label.setFont(*font);
                label.setCharacterSize(13);
                label.setFillColor(theme::TEXT_MAIN);

                std::string displayName = entry.path.filename().string();
                if (entry.path == getCurrentPath().parent_path() && getCurrentPath().has_parent_path())
                    displayName = "..";
                else if (entry.isDirectory)
                    displayName += "/";

                label.setPosition({LEFT_X + 10.f, rowY + 4.f});
                label.setString(displayName);
                window.draw(label);

                label.setPosition({LEFT_X + 713.f, rowY + 4.f});
                label.setString(entry.isDirectory ? std::to_string(entry.size) + " items" : std::to_string(entry.size) + " bytes");
                window.draw(label);

                label.setPosition({LEFT_X + 843.f, rowY + 4.f});
                label.setString(TimestampUtils::toString(entry.lastModified, "%d %b %Y"));
                window.draw(label);
            }

            sf::RectangleShape headerMask;
            headerMask.setPosition({16.f, 16.f});
            headerMask.setSize({static_cast<float>(windowWidth) - 32.f, LIST_TOP - 16.f});
            headerMask.setFillColor(theme::BG_PANEL);
            window.draw(headerMask);

            backButton.layout(LEFT_X, 24.f, 34.f, 28.f);
            window.draw(backButton);

            nextButton.layout(LEFT_X + 40.f, 24.f, 34.f, 28.f);
            window.draw(nextButton);

            currentPathField.layout(LEFT_X + 80.f, 24.f, 825.f, 28.f);
            if (!currentPathField.focused)
                currentPathField.setValue(getCurrentPath().string());
            window.draw(currentPathField);

            upButton.layout(LEFT_X + 910.f, 24.f, 34.f, 28.f);
            window.draw(upButton);
            drawTriangle(LEFT_X + 910.f + 17.f, 24.f + 14.f, true, 6.f, 4.f);

            nameSortButton.layout(LEFT_X, 68.f, 683.f, 28.f);
            window.draw(nameSortButton);

            sizeSortButton.layout(LEFT_X + 693.f, 68.f, 120.f, 28.f);
            window.draw(sizeSortButton);

            modifiedSortButton.layout(LEFT_X + 823.f, 68.f, 120.f, 28.f);
            window.draw(modifiedSortButton);

            if (sortColumn == SortColumn::NAME)
                drawSortArrow(LEFT_X, 683.f, 68.f);
            else if (sortColumn == SortColumn::SIZE)
                drawSortArrow(LEFT_X + 693.f, 120.f, 68.f);
            else
                drawSortArrow(LEFT_X + 823.f, 120.f, 68.f);

            separator.layout(LEFT_X, 103.f, LIST_WIDTH);
            window.draw(separator);

            const float buttonWidth = 120.f;
            const float buttonHeight = 34.f;
            const float fieldHeight = 28.f;
            const float rowGap = 10.f;

            const float rowY = static_cast<float>(windowHeight) - 32.f - buttonHeight;
            const float sepY = rowY - 16.f;
            const float contentRight = LEFT_X + LIST_WIDTH;

            sf::RectangleShape footer;
            footer.setPosition({16.f, sepY - 8.f});
            footer.setSize({static_cast<float>(windowWidth) - 32.f, static_cast<float>(windowHeight) - 16.f - (sepY - 8.f)});
            footer.setFillColor(theme::BG_PANEL);
            window.draw(footer);

            separator.layout(LEFT_X, sepY, LIST_WIDTH);
            window.draw(separator);

            const float buttonX = contentRight - buttonWidth;
            const float fieldWidth = buttonX - rowGap - LEFT_X;

            if (mode == ExploratorMode::OPEN && selectedEntry != static_cast<std::size_t>(-1)
                && !entries[selectedEntry].isDirectory)
                filenameField.setValue(entries[selectedEntry].path.filename().string());

            filenameField.layout(LEFT_X, rowY + (buttonHeight - fieldHeight) / 2.f, fieldWidth, fieldHeight);
            window.draw(filenameField);

            saveButton.layout(buttonX, rowY, buttonWidth, buttonHeight);
            window.draw(saveButton);
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
                const float visibleBottom = listBottom();

                if (mouse.y >= LIST_TOP && mouse.y <= visibleBottom)
                {
                    for (std::size_t index = 0; index < entries.size(); ++index)
                    {
                        const float rowY = LIST_TOP + scrollOffset + static_cast<float>(index) * ROW_PITCH;

                        if (rowY + ROW_HEIGHT < LIST_TOP || rowY > visibleBottom)
                            continue;

                        const sf::FloatRect rowBounds{LEFT_X, rowY, LIST_WIDTH, ROW_HEIGHT};

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
                    }
                }
            }

            if (event.type == sf::Event::MouseWheelScrolled)
            {
                if (event.mouseWheelScroll.wheel == sf::Mouse::VerticalWheel)
                {
                    scrollOffset += event.mouseWheelScroll.delta * 30.f;
                    scrollOffset = std::min(0.f, scrollOffset);
                    scrollOffset = std::max(-maxScroll, scrollOffset);
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