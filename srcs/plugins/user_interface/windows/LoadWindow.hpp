/*
* ____  _ _____   ___  _   _   _   _  _         ___ _____ _  _   _   _  _
* |_ _| _ |_   _|/ _ \| | | | /_\ | \| |   <3  | __|_   _| || | /_\ | \| |
*  | | | |  | | | (_) | |_| |/ _ \| .` |   <3  | _|  | | | __ |/ _ \| .` |
*  |_| |_|  |_|  \___/ \___//_/ \_\_|\_|   <3  |___| |_| |_||_/_/ \_\_|\_|
*
* -----------------------------------------------------------------------
* File:    LoadWindow.hpp
* Who:     Titouan & Ethan
* Date:    2026-05-04
* -----------------------------------------------------------------------
*/

#ifndef LOADWINDOW_HPP
#define LOADWINDOW_HPP

#define SPACE_WIDTH 300.f

#include <atomic>
#include <thread>
#include <iostream>
#include <vector>
#include <cstdio>
#include <algorithm>
#include <SFML/Graphics.hpp>

#include "../../../common/scene/IScene.hpp"
#include "../Theme.hpp"
#include "../LayoutPen.hpp"
#include "../components/Button.hpp"
#include "../components/Checkbox.hpp"
#include "../components/Separator.hpp"
#include "Window.hpp"

namespace rc
{
    struct LoadWindow : Window
    {
        IScene *scene;
        IScene *loadedScene;

        bool confirmed = false;

        Button saveButton;
        Separator separator1;
        Separator separator2;
        Separator separator3;

        Checkbox cameraCheckbox;
        Checkbox cameraPositionCheckbox;
        Checkbox cameraResolutionCheckbox;
        Checkbox cameraRotationCheckbox;
        Checkbox cameraFovCheckbox;
        Checkbox cameraSppCheckbox;
        Checkbox lightsCheckbox;
        Checkbox lightAmbientCheckbox;
        Checkbox lightDiffuseCheckbox;

        Checkbox loadCameraCheckbox;
        Checkbox loadCameraPositionCheckbox;
        Checkbox loadCameraResolutionCheckbox;
        Checkbox loadCameraRotationCheckbox;
        Checkbox loadCameraFovCheckbox;
        Checkbox loadCameraSppCheckbox;
        Checkbox loadLightsCheckbox;
        Checkbox loadLightAmbientCheckbox;
        Checkbox loadLightDiffuseCheckbox;

        unsigned int calculateWindowHeight()
        {
            unsigned int height = 84;

            height += 26;
            height += 3 * 24;
            height += 22;

            height += 24;
            height += 30;

            if (scene->getCamera().getPosition() != loadedScene->getCamera().getPosition())
                height += 26 + 3 * 20;
            if (scene->getCamera().getResolution() != loadedScene->getCamera().getResolution())
                height += 26 + 2 * 20;
            if (scene->getCamera().getRotation() != loadedScene->getCamera().getRotation())
                height += 26 + 3 * 20;
            if (scene->getCamera().getFov() != loadedScene->getCamera().getFov())
                height += 26 + 20;
            if (scene->getCamera().getSamplesPerPixel() != loadedScene->getCamera().getSamplesPerPixel())
                height += 26 + 20;

            height += 28;

            height += 30;
            if (scene->getAmbientCoefficient() != loadedScene->getAmbientCoefficient())
                height += 26 + 20;
            if (scene->getDiffuseCoefficient() != loadedScene->getDiffuseCoefficient())
                height += 26 + 20;

            height += 28;

            height += 26;
            std::size_t matRows = std::max(scene->getMaterials().size(),
                                           loadedScene->getMaterials().size());
            height += static_cast<unsigned int>(matRows) * 52;

            height += 150;

            return height;
        }

        void create(sf::Font &f, IScene &s, IScene &ls)
        {
            font = &f;
            scene = &s;
            loadedScene = &ls;

            windowHeight = calculateWindowHeight();
            windowWidth = 620;
            windowTitle = "Import scene";

            confirmed = false;
            running = true;
            thread = std::thread(&LoadWindow::loop, this);

            saveButton.setFont(*font);
            saveButton.setLabel("Import selected");
            saveButton.onClick = [&]
            {
                confirmed = true;
                destroy();
            };

            cameraCheckbox.box.setSize({18, 18});
            cameraCheckbox.setFont(*font);
            cameraCheckbox.setLabel("Camera");
            cameraCheckbox.setChecked(true);
            cameraCheckbox.onToggle = [&](bool v)
            {
                cameraPositionCheckbox.setChecked(v);
                cameraResolutionCheckbox.setChecked(v);
                cameraRotationCheckbox.setChecked(v);
                cameraFovCheckbox.setChecked(v);
                cameraSppCheckbox.setChecked(v);

                loadCameraCheckbox.setChecked(!v);
                loadCameraPositionCheckbox.setChecked(!v);
                loadCameraResolutionCheckbox.setChecked(!v);
                loadCameraRotationCheckbox.setChecked(!v);
                loadCameraFovCheckbox.setChecked(!v);
                loadCameraSppCheckbox.setChecked(!v);
            };

            cameraPositionCheckbox.box.setSize({18, 18});
            cameraPositionCheckbox.setFont(*font);
            cameraPositionCheckbox.setLabel("Position");
            cameraPositionCheckbox.setChecked(true);
            cameraPositionCheckbox.onToggle = [&](bool v)
            {
                loadCameraPositionCheckbox.setChecked(!v);
            };

            cameraResolutionCheckbox.box.setSize({18, 18});
            cameraResolutionCheckbox.setFont(*font);
            cameraResolutionCheckbox.setLabel("Resolution");
            cameraResolutionCheckbox.setChecked(true);
            cameraResolutionCheckbox.onToggle = [&](bool v)
            {
                loadCameraResolutionCheckbox.setChecked(!v);
            };

            cameraRotationCheckbox.box.setSize({18, 18});
            cameraRotationCheckbox.setFont(*font);
            cameraRotationCheckbox.setLabel("Rotation");
            cameraRotationCheckbox.setChecked(true);
            cameraRotationCheckbox.onToggle = [&](bool v)
            {
                loadCameraRotationCheckbox.setChecked(!v);
            };

            cameraFovCheckbox.box.setSize({18, 18});
            cameraFovCheckbox.setFont(*font);
            cameraFovCheckbox.setLabel("fieldOfView");
            cameraFovCheckbox.setChecked(true);
            cameraFovCheckbox.onToggle = [&](bool v)
            {
                loadCameraFovCheckbox.setChecked(!v);
            };

            cameraSppCheckbox.box.setSize({18, 18});
            cameraSppCheckbox.setFont(*font);
            cameraSppCheckbox.setLabel("samplesPerPixel");
            cameraSppCheckbox.setChecked(true);
            cameraSppCheckbox.onToggle = [&](bool v)
            {
                loadCameraSppCheckbox.setChecked(!v);
            };

            lightsCheckbox.box.setSize({18, 18});
            lightsCheckbox.setFont(*font);
            lightsCheckbox.setLabel("Lights");
            lightsCheckbox.setChecked(true);
            lightsCheckbox.onToggle = [&](bool v)
            {
                loadLightsCheckbox.setChecked(!v);
                loadLightAmbientCheckbox.setChecked(!v);
                loadLightDiffuseCheckbox.setChecked(!v);

                lightAmbientCheckbox.setChecked(v);
                lightDiffuseCheckbox.setChecked(v);
            };

            lightAmbientCheckbox.box.setSize({18, 18});
            lightAmbientCheckbox.setFont(*font);
            lightAmbientCheckbox.setLabel("Ambient coefficient");
            lightAmbientCheckbox.setChecked(true);
            lightAmbientCheckbox.onToggle = [&](bool v)
            {
                loadLightAmbientCheckbox.setChecked(!v);
            };

            lightDiffuseCheckbox.box.setSize({18, 18});
            lightDiffuseCheckbox.setFont(*font);
            lightDiffuseCheckbox.setLabel("Diffuse coefficient");
            lightDiffuseCheckbox.setChecked(true);
            lightDiffuseCheckbox.onToggle = [&](bool v)
            {
                loadLightDiffuseCheckbox.setChecked(!v);
            };

            loadCameraCheckbox.box.setSize({18, 18});
            loadCameraCheckbox.setFont(*font);
            loadCameraCheckbox.setLabel("Camera");
            loadCameraCheckbox.setChecked(false);
            loadCameraCheckbox.onToggle = [&](bool v)
            {
                loadCameraPositionCheckbox.setChecked(v);
                loadCameraResolutionCheckbox.setChecked(v);
                loadCameraRotationCheckbox.setChecked(v);
                loadCameraFovCheckbox.setChecked(v);
                loadCameraSppCheckbox.setChecked(v);

                cameraCheckbox.setChecked(!v);
                cameraPositionCheckbox.setChecked(!v);
                cameraResolutionCheckbox.setChecked(!v);
                cameraRotationCheckbox.setChecked(!v);
                cameraFovCheckbox.setChecked(!v);
                cameraSppCheckbox.setChecked(!v);
            };

            loadCameraPositionCheckbox.box.setSize({18, 18});
            loadCameraPositionCheckbox.setFont(*font);
            loadCameraPositionCheckbox.setLabel("Position");
            loadCameraPositionCheckbox.setChecked(false);
            loadCameraPositionCheckbox.onToggle = [&](bool v)
            {
                cameraPositionCheckbox.setChecked(!v);
            };

            loadCameraResolutionCheckbox.box.setSize({18, 18});
            loadCameraResolutionCheckbox.setFont(*font);
            loadCameraResolutionCheckbox.setLabel("Resolution");
            loadCameraResolutionCheckbox.setChecked(false);
            loadCameraResolutionCheckbox.onToggle = [&](bool v)
            {
                cameraResolutionCheckbox.setChecked(!v);
            };

            loadCameraRotationCheckbox.box.setSize({18, 18});
            loadCameraRotationCheckbox.setFont(*font);
            loadCameraRotationCheckbox.setLabel("Rotation");
            loadCameraRotationCheckbox.setChecked(false);
            loadCameraRotationCheckbox.onToggle = [&](bool v)
            {
                cameraRotationCheckbox.setChecked(!v);
            };

            loadCameraFovCheckbox.box.setSize({18, 18});
            loadCameraFovCheckbox.setFont(*font);
            loadCameraFovCheckbox.setLabel("fieldOfView");
            loadCameraFovCheckbox.setChecked(false);
            loadCameraFovCheckbox.onToggle = [&](bool v)
            {
                cameraFovCheckbox.setChecked(!v);
            };

            loadCameraSppCheckbox.box.setSize({18, 18});
            loadCameraSppCheckbox.setFont(*font);
            loadCameraSppCheckbox.setLabel("samplesPerPixel");
            loadCameraSppCheckbox.setChecked(false);
            loadCameraSppCheckbox.onToggle = [&](bool v)
            {
                cameraSppCheckbox.setChecked(!v);
            };

            loadLightsCheckbox.box.setSize({18, 18});
            loadLightsCheckbox.setFont(*font);
            loadLightsCheckbox.setLabel("Lights");
            loadLightsCheckbox.setChecked(false);
            loadLightsCheckbox.onToggle = [&](bool v)
            {
                lightsCheckbox.setChecked(!v);
                lightAmbientCheckbox.setChecked(!v);
                lightDiffuseCheckbox.setChecked(!v);

                loadLightAmbientCheckbox.setChecked(v);
                loadLightDiffuseCheckbox.setChecked(v);
            };

            loadLightAmbientCheckbox.box.setSize({18, 18});
            loadLightAmbientCheckbox.setFont(*font);
            loadLightAmbientCheckbox.setLabel("Ambient coefficient");
            loadLightAmbientCheckbox.setChecked(false);
            loadLightAmbientCheckbox.onToggle = [&](bool v)
            {
                lightAmbientCheckbox.setChecked(!v);
            };

            loadLightDiffuseCheckbox.box.setSize({18, 18});
            loadLightDiffuseCheckbox.setFont(*font);
            loadLightDiffuseCheckbox.setLabel("Diffuse coefficient");
            loadLightDiffuseCheckbox.setChecked(false);
            loadLightDiffuseCheckbox.onToggle = [&](bool v)
            {
                lightDiffuseCheckbox.setChecked(!v);
            };
        }


        static std::string fmt(float v)
        {
            char buf[32];
            std::snprintf(buf, sizeof(buf), "%.2f", v);
            return std::string(buf);
        }

        void drawColoredText(const std::string &text, float x, float y,
                             unsigned int size, const sf::Color &color)
        {
            sf::Text t;
            t.setFont(*font);
            t.setString(text);
            t.setCharacterSize(size);
            t.setFillColor(color);
            t.setPosition({x, y});
            window.draw(t);
        }

        void drawRect(float x, float y, float w, float h, const sf::Color &fill,
                      float outline = 0.f, const sf::Color &outlineColor = theme::OUTLINE)
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

        void drawSectionHeader(const std::string &text, float x, float y)
        {
            drawRect(x, y + 1.f, 3.f, 16.f, theme::ACCENT);
            drawColoredText(text, x + 12.f, y, 15, theme::TEXT_SUBTLE);
        }

        std::string materialProps(const Material &m)
        {
            if (m.model == MaterialModel::PBR)
                return "metallic " + fmt(m.metallic) + "   roughness " + fmt(m.roughness) +
                       "   ior " + fmt(m.ior);
            return "shininess " + fmt(m.shininess) + "   refl " + fmt(m.reflectivity) +
                   "   transp " + fmt(m.transparency);
        }

        void drawMaterialEntry(const Material &m, float x, float y)
        {
            Color c = m.getBaseColor().toColor();

            drawRect(x, y, 20.f, 20.f, sf::Color(c.r, c.g, c.b), 1.f, theme::OUTLINE_MID);

            drawColoredText(m.getName(), x + 30.f, y - 1.f, 15, theme::TEXT_MAIN);

            std::string model = m.getModelName();
            std::transform(model.begin(), model.end(), model.begin(), ::toupper);
            drawColoredText(model, x + SPACE_WIDTH - 62.f, y + 1.f, 12, theme::ACCENT);

            drawColoredText(materialProps(m), x + 30.f, y + 19.f, 12, theme::TEXT_DIM);
        }

        void drawStatRow(const std::string &label, std::size_t current,
                         std::size_t loaded, float x, float y)
        {
            drawColoredText(label, x, y, 14, theme::TEXT_SUBTLE);

            sf::Color loadedColor = (current != loaded) ? theme::ACCENT : theme::TEXT_MAIN;
            drawColoredText(std::to_string(current), x + 170.f, y, 14, theme::TEXT_MAIN);
            drawColoredText("->", x + 210.f, y, 14, theme::TEXT_DIM);
            drawColoredText(std::to_string(loaded), x + 245.f, y, 14, loadedColor);
        }

        void drawUi()
        {
            const float contentWidth = static_cast<float>(windowWidth) - 40.f;

            drawRect(0.f, 0.f, static_cast<float>(windowWidth), 64.f, theme::BG_BAR);
            drawRect(0.f, 64.f, static_cast<float>(windowWidth), 2.f, theme::ACCENT);
            drawColoredText("Import Scene", 20.f, 16.f, 24, theme::TEXT_WHITE);
            drawColoredText("Review the differences and choose what to keep",
                            20.f, 44.f, 13, theme::TEXT_DIM);

            LayoutPen layout{20.f, 84.f, 6.f};

            drawSectionHeader("OVERVIEW", layout.x, layout.y);
            layout.next(20);

            drawStatRow("Objects", scene->getPrimitives().size(),
                        loadedScene->getPrimitives().size(), layout.x, layout.y);
            layout.next(18);
            drawStatRow("Lights", scene->getLights().size(),
                        loadedScene->getLights().size(), layout.x, layout.y);
            layout.next(18);
            drawStatRow("Materials", scene->getMaterials().size(),
                        loadedScene->getMaterials().size(), layout.x, layout.y);
            layout.next(20);

            separator1.layout(layout.x, layout.y, contentWidth);
            window.draw(separator1);
            layout.next(10);

            drawColoredText("CURRENT SCENE", layout.x, layout.y, 14, theme::TEXT_SUBTLE);
            drawColoredText("LOAD SCENE", layout.x + SPACE_WIDTH, layout.y, 14, theme::TEXT_SUBTLE);
            layout.next(24);

            cameraCheckbox.layout(layout.x, layout.y);
            window.draw(cameraCheckbox);
            loadCameraCheckbox.layout(layout.x + SPACE_WIDTH, layout.y);
            window.draw(loadCameraCheckbox);
            layout.next(24);

            layout.x += 20.f;

            if (scene->getCamera().getPosition() != loadedScene->getCamera().getPosition())
            {
                cameraPositionCheckbox.layout(layout.x, layout.y);
                window.draw(cameraPositionCheckbox);
                loadCameraPositionCheckbox.layout(layout.x + SPACE_WIDTH, layout.y);
                window.draw(loadCameraPositionCheckbox);
                layout.next(20);

                layout.x += 20.f;

                drawText("x = " + std::to_string(scene->getCamera().getPosition().x), layout);
                drawText("x = " + std::to_string(loadedScene->getCamera().getPosition().x), {layout.x + SPACE_WIDTH, layout.y, layout.spacing});
                layout.next(18);
            
                drawText("y = " + std::to_string(scene->getCamera().getPosition().y), layout);
                drawText("y = " + std::to_string(loadedScene->getCamera().getPosition().y), {layout.x + SPACE_WIDTH, layout.y, layout.spacing});
                layout.next(18);

                drawText("z = " + std::to_string(scene->getCamera().getPosition().z), layout);
                drawText("z = " + std::to_string(loadedScene->getCamera().getPosition().z), {layout.x + SPACE_WIDTH, layout.y, layout.spacing});
                layout.next(18);

                layout.x -= 20.f;
            }

            if (scene->getCamera().getResolution() != loadedScene->getCamera().getResolution())
            {
                cameraResolutionCheckbox.layout(layout.x, layout.y);
                window.draw(cameraResolutionCheckbox);
                loadCameraResolutionCheckbox.layout(layout.x + SPACE_WIDTH, layout.y);
                window.draw(loadCameraResolutionCheckbox);
                layout.next(20);

                layout.x += 20.f;

                drawText("width = " + std::to_string(scene->getCamera().getResolution().x), layout);
                drawText("width = " + std::to_string(loadedScene->getCamera().getResolution().x), {layout.x + SPACE_WIDTH, layout.y, layout.spacing});
                layout.next(18);
            
                drawText("height = " + std::to_string(scene->getCamera().getResolution().y), layout);
                drawText("height = " + std::to_string(loadedScene->getCamera().getResolution().y), {layout.x + SPACE_WIDTH, layout.y, layout.spacing});
                layout.next(18);

                layout.x -= 20.f;
            }

            if (scene->getCamera().getRotation() != loadedScene->getCamera().getRotation())
            {
                cameraRotationCheckbox.layout(layout.x, layout.y);
                window.draw(cameraRotationCheckbox);
                loadCameraRotationCheckbox.layout(layout.x + SPACE_WIDTH, layout.y);
                window.draw(loadCameraRotationCheckbox);
                layout.next(20);

                layout.x += 20.f;

                drawText("x = " + std::to_string(scene->getCamera().getRotation().x), layout);
                drawText("x = " + std::to_string(loadedScene->getCamera().getRotation().x), {layout.x + SPACE_WIDTH, layout.y, layout.spacing});
                layout.next(18);
            
                drawText("y = " + std::to_string(scene->getCamera().getRotation().y), layout);
                drawText("y = " + std::to_string(loadedScene->getCamera().getRotation().y), {layout.x + SPACE_WIDTH, layout.y, layout.spacing});
                layout.next(18);

                drawText("z = " + std::to_string(scene->getCamera().getRotation().z), layout);
                drawText("z = " + std::to_string(loadedScene->getCamera().getRotation().z), {layout.x + SPACE_WIDTH, layout.y, layout.spacing});
                layout.next(18);

                layout.x -= 20.f;
            }

            if (scene->getCamera().getFov() != loadedScene->getCamera().getFov())
            {
                cameraFovCheckbox.layout(layout.x, layout.y);
                window.draw(cameraFovCheckbox);
                loadCameraFovCheckbox.layout(layout.x + SPACE_WIDTH, layout.y);
                window.draw(loadCameraFovCheckbox);
                layout.next(20);

                layout.x += 20.f;

                drawText(std::to_string(scene->getCamera().getFov()), layout);
                drawText(std::to_string(loadedScene->getCamera().getFov()), {layout.x + SPACE_WIDTH, layout.y, layout.spacing});
                layout.next(18);

                layout.x -= 20.f;
            }

            if (scene->getCamera().getSamplesPerPixel() != loadedScene->getCamera().getSamplesPerPixel())
            {
                cameraSppCheckbox.layout(layout.x, layout.y);
                window.draw(cameraSppCheckbox);
                loadCameraSppCheckbox.layout(layout.x + SPACE_WIDTH, layout.y);
                window.draw(loadCameraSppCheckbox);
                layout.next(20);

                layout.x += 20.f;

                drawText(std::to_string(scene->getCamera().getSamplesPerPixel()), layout);
                drawText(std::to_string(loadedScene->getCamera().getSamplesPerPixel()), {layout.x + SPACE_WIDTH, layout.y, layout.spacing});
                layout.next(18);

                layout.x -= 20.f;
            }

            layout.x -= 20.f;
            layout.next(12);

            separator2.layout(layout.x, layout.y, contentWidth);
            window.draw(separator2);
            layout.next(12);

            lightsCheckbox.layout(layout.x, layout.y);
            window.draw(lightsCheckbox);
            loadLightsCheckbox.layout(layout.x + SPACE_WIDTH, layout.y);
            window.draw(loadLightsCheckbox);
            layout.next(24);

            if (scene->getAmbientCoefficient() != loadedScene->getAmbientCoefficient())
            {
                layout.x += 20.f;

                lightAmbientCheckbox.layout(layout.x, layout.y);
                window.draw(lightAmbientCheckbox);
                loadLightAmbientCheckbox.layout(layout.x + SPACE_WIDTH, layout.y);
                window.draw(loadLightAmbientCheckbox);
                layout.next(20);

                layout.x += 20.f;

                drawText(std::to_string(scene->getAmbientCoefficient()), layout);
                drawText(std::to_string(loadedScene->getAmbientCoefficient()), {layout.x + SPACE_WIDTH, layout.y, layout.spacing});
                layout.next(18);

                layout.x -= 40.f;
            }

            if (scene->getDiffuseCoefficient() != loadedScene->getDiffuseCoefficient())
            {
                layout.x += 20.f;

                lightDiffuseCheckbox.layout(layout.x, layout.y);
                window.draw(lightDiffuseCheckbox);
                loadLightDiffuseCheckbox.layout(layout.x + SPACE_WIDTH, layout.y);
                window.draw(loadLightDiffuseCheckbox);
                layout.next(20);

                layout.x += 20.f;

                drawText(std::to_string(scene->getDiffuseCoefficient()), layout);
                drawText(std::to_string(loadedScene->getDiffuseCoefficient()), {layout.x + SPACE_WIDTH, layout.y, layout.spacing});
                layout.next(18);

                layout.x -= 40.f;
            }

            layout.next(12);

            separator3.layout(layout.x, layout.y, contentWidth);
            window.draw(separator3);
            layout.next(12);

            drawSectionHeader("MATERIALS", layout.x, layout.y);
            layout.next(24);

            const auto &currentMaterials = scene->getMaterials();
            const auto &loadMaterials = loadedScene->getMaterials();

            std::vector<const Material *> currentList;
            std::vector<const Material *> loadList;
            for (const auto &pair : currentMaterials)
                currentList.push_back(&pair.second);
            for (const auto &pair : loadMaterials)
                loadList.push_back(&pair.second);

            std::size_t rows = std::max(currentList.size(), loadList.size());
            if (rows == 0)
            {
                drawColoredText("No materials defined.", layout.x, layout.y, 13, theme::TEXT_DIM);
                layout.next(20);
            }
            for (std::size_t i = 0; i < rows; ++i)
            {
                if (i < currentList.size())
                    drawMaterialEntry(*currentList[i], layout.x, layout.y);
                if (i < loadList.size())
                    drawMaterialEntry(*loadList[i], layout.x + SPACE_WIDTH, layout.y);
                layout.next(40);
            }

            layout.next(12);

            saveButton.layout(layout.x, layout.y, contentWidth, 50);
            window.draw(saveButton);

            fitHeightToContent(layout.y + 50.f + 20.f);
        }

        void fitHeightToContent(float contentBottom)
        {
            unsigned int needed = static_cast<unsigned int>(contentBottom);

            if (needed == windowHeight)
                return;
            windowHeight = needed;
            window.setSize({windowWidth, windowHeight});
            window.setView(sf::View(sf::FloatRect(0.f, 0.f,
                static_cast<float>(windowWidth), static_cast<float>(windowHeight))));
        }

        void updateUi()
        {
            sf::Vector2i mouse = sf::Mouse::getPosition(window);

            cameraCheckbox.update(mouse);
            cameraPositionCheckbox.update(mouse);
            cameraResolutionCheckbox.update(mouse);
            cameraRotationCheckbox.update(mouse);
            cameraFovCheckbox.update(mouse);
            cameraSppCheckbox.update(mouse);

            lightsCheckbox.update(mouse);
            lightAmbientCheckbox.update(mouse);
            lightDiffuseCheckbox.update(mouse);

            loadCameraCheckbox.update(mouse);
            loadCameraPositionCheckbox.update(mouse);
            loadCameraResolutionCheckbox.update(mouse);
            loadCameraRotationCheckbox.update(mouse);
            loadCameraFovCheckbox.update(mouse);
            loadCameraSppCheckbox.update(mouse);

            loadLightsCheckbox.update(mouse);
            loadLightAmbientCheckbox.update(mouse);
            loadLightDiffuseCheckbox.update(mouse);
        }

        void handleEvent(const sf::Event &event)
        {
            sf::Vector2i mouse = sf::Mouse::getPosition(window);

            saveButton.handleEvent(event, mouse);

            cameraCheckbox.handleEvent(event, mouse);
            cameraPositionCheckbox.handleEvent(event, mouse);
            cameraResolutionCheckbox.handleEvent(event, mouse);
            cameraRotationCheckbox.handleEvent(event, mouse);
            cameraFovCheckbox.handleEvent(event, mouse);
            cameraSppCheckbox.handleEvent(event, mouse);

            lightsCheckbox.handleEvent(event, mouse);
            lightAmbientCheckbox.handleEvent(event, mouse);
            lightDiffuseCheckbox.handleEvent(event, mouse);

            loadCameraCheckbox.handleEvent(event, mouse);
            loadCameraPositionCheckbox.handleEvent(event, mouse);
            loadCameraResolutionCheckbox.handleEvent(event, mouse);
            loadCameraRotationCheckbox.handleEvent(event, mouse);
            loadCameraFovCheckbox.handleEvent(event, mouse);
            loadCameraSppCheckbox.handleEvent(event, mouse);

            loadLightsCheckbox.handleEvent(event, mouse);
            loadLightAmbientCheckbox.handleEvent(event, mouse);
            loadLightDiffuseCheckbox.handleEvent(event, mouse);
        }
    };
}

#endif
