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
#include <SFML/Graphics.hpp>

#include "../../../common/scene/IScene.hpp"
#include "../Theme.hpp"
#include "../VerticalLayout.hpp"
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

        Button saveButton;
        Separator separator1;
        Separator separator2;

        // Checkboxes for current scene state
        Checkbox cameraCheckbox;
        Checkbox cameraPositionCheckbox;
        Checkbox cameraResolutionCheckbox;
        Checkbox cameraRotationCheckbox;
        Checkbox cameraFovCheckbox;
        Checkbox cameraSppCheckbox;
        Checkbox lightsCheckbox;
        Checkbox lightAmbientCheckbox;
        Checkbox lightDiffuseCheckbox;

        // Checkboxes for load scene
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
            unsigned int height = 150; // top + bottom margins

            height += 40;
            height += 30;
            height += 30;

            if (scene->getCamera().getPosition() != loadedScene->getCamera().getPosition())
                height += 80;
            if (scene->getCamera().getResolution() != loadedScene->getCamera().getResolution())
                height += 70;
            if (scene->getCamera().getRotation() != loadedScene->getCamera().getRotation())
                height += 80;
            if (scene->getCamera().getFov() != loadedScene->getCamera().getFov())
                height += 50;
            if (scene->getCamera().getSamplesPerPixel() != loadedScene->getCamera().getSamplesPerPixel())
                height += 50;

            height += 30;
            height += 30;

            if (scene->getAmbientCoefficient() != loadedScene->getAmbientCoefficient())
                height += 50;
            if (scene->getDiffuseCoefficient() != loadedScene->getDiffuseCoefficient())
                height += 50;

            height += 100;

            return height;
        }

        void create(sf::Font &f, IScene &s, IScene &ls)
        {
            font = &f;
            scene = &s;
            loadedScene = &ls;

            windowHeight = calculateWindowHeight();
            windowWidth = 540;
            windowTitle = "Load scene";

            running = true;
            thread = std::thread(&LoadWindow::loop, this);

            saveButton.setFont(*font);
            saveButton.setLabel("Save scene");
            saveButton.onClick = [&]
            {
                if (loadCameraPositionCheckbox.checked)
                    scene->getCamera().setPosition(loadedScene->getCamera().getPosition());
                if (loadCameraResolutionCheckbox.checked)
                    scene->getCamera().setResolution(loadedScene->getCamera().getResolution());
                if (loadCameraRotationCheckbox.checked)
                    scene->getCamera().setRotation(loadedScene->getCamera().getRotation());
                if (loadCameraFovCheckbox.checked)
                    scene->getCamera().setFov(loadedScene->getCamera().getFov());
                if (loadCameraSppCheckbox.checked)
                    scene->getCamera().setSamplesPerPixel(loadedScene->getCamera().getSamplesPerPixel());

                if (loadLightAmbientCheckbox.checked)
                    scene->setAmbientCoefficient(loadedScene->getAmbientCoefficient());
                if (loadLightDiffuseCheckbox.checked)
                    scene->setDiffuseCoefficient(loadedScene->getDiffuseCoefficient());
                
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

        void drawUi()
        {
            VerticalLayout layout{20.f, 20.f, 16.f};

            // === HEADER SECTION ===
            sf::Text headerText;
            headerText.setFont(*font);
            headerText.setString("Scene Comparison");
            headerText.setCharacterSize(22);
            headerText.setFillColor(theme::ACCENT);
            headerText.setPosition({layout.x, layout.y});
            window.draw(headerText);
            layout.next(28);

            // Visual separator
            separator1.layout(layout.x, layout.y, 500.f);
            window.draw(separator1);
            layout.next(8);

            // === COLUMNS HEADER ===
            sf::Text currentHeader;
            currentHeader.setFont(*font);
            currentHeader.setString("CURRENT SCENE");
            currentHeader.setCharacterSize(14);
            currentHeader.setFillColor(theme::TEXT_SUBTLE);
            currentHeader.setPosition({layout.x, layout.y});
            window.draw(currentHeader);

            sf::Text loadHeader;
            loadHeader.setFont(*font);
            loadHeader.setString("LOAD SCENE");
            loadHeader.setCharacterSize(14);
            loadHeader.setFillColor(theme::TEXT_SUBTLE);
            loadHeader.setPosition({layout.x + SPACE_WIDTH, layout.y});
            window.draw(loadHeader);
            layout.next(24);

            // === CAMERA SECTION ===
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

            // Visual separator
            separator2.layout(layout.x, layout.y, 500.f);
            window.draw(separator2);
            layout.next(12);

            // === LIGHTS SECTION ===
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

            // === ACTION BUTTON ===
            saveButton.layout(layout.x, layout.y, 500.f, 50);
            window.draw(saveButton);
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
