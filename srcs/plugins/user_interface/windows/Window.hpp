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
 *  │ File   : Window.hpp
 *  │ Author : Tataneeeeeeeeeee
 *  │ Date   : 2026-05-13
 *  │ Brief  : Window de folie
 *  └────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────┘
*/

#ifndef WINDOW_HPP
#define WINDOW_HPP

#include <atomic>
#include <thread>
#include <iostream>
#include <SFML/Graphics.hpp>

#include "../Theme.hpp"
#include "../VerticalLayout.hpp"


namespace rc
{
    struct Window
    {
        sf::RenderWindow  window;
        std::thread       thread;
        std::atomic<bool> running = false;

        sf::Font *font = nullptr;

        unsigned int windowWidth = 600;
        unsigned int windowHeight = 400;
        std::string windowTitle = "Window";

        Window() = default;

        virtual ~Window()
        {
            destroy();
        }

        virtual void handleEvent(const sf::Event &event) = 0;
        virtual void drawUi() = 0;
        virtual void updateUi() = 0;

        void destroy()
        {
            this->running = false;
            // if (window.isOpen())
                // window.close();

            if (thread.get_id() == std::this_thread::get_id())
            {
                if (thread.joinable())
                    thread.detach();
                return;
            }

            if (thread.joinable())
                thread.join();
        }
        
        void drawTitle(const std::string &text, VerticalLayout layout)
        {
            sf::Text title;
            title.setFont(*font);
            title.setString(text);
            title.setCharacterSize(20);
            title.setFillColor(theme::TEXT_DIM);
            title.setPosition({layout.x, layout.y});
            window.draw(title);
        }

        void drawText(const std::string &text, VerticalLayout layout)
        {
            sf::Text title;
            title.setFont(*font);
            title.setString(text);
            title.setCharacterSize(14);
            title.setFillColor(theme::TEXT_DIM);
            title.setPosition({layout.x, layout.y});
            window.draw(title);
        }

        void loop()
        {
            this->window.create({windowWidth, windowHeight}, windowTitle, this->_windowStyle);

            while (this->running)
            {
                sf::Event event;
                while (window.pollEvent(event) && this->running)
                {
                    if (event.type == sf::Event::Closed)
                    {
                        window.close();
                        destroy();
                        return;
                    }
                    if (event.type == sf::Event::Resized)
                    {
                        sf::FloatRect visibleArea(0, 0, event.size.width, event.size.height);
                        this->window.setView(sf::View(visibleArea));
                    }
                    handleEvent(event);
                }

                if (!this->running)
                    break;

                window.clear(theme::BG_WINDOW);

                updateUi();
                drawUi();

                window.display();
            }
            window.close();
        }
        protected:
            sf::Uint32 _windowStyle = sf::Style::Default;
    };
}

#endif