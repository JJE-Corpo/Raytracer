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
#include <chrono>
#include <mutex>
#include <thread>
#include <iostream>
#include <SFML/Graphics.hpp>

#include "../Theme.hpp"
#include "../LayoutPen.hpp"


namespace rc
{
    // Every SFML window in this process shares ONE X11 Display connection. The
    // GLX driver runs an XSync (an X round-trip) inside display(), and when the
    // main UI window thread and a pop-up window thread hit that shared
    // connection at the same time the xcb request sequence is lost - which
    // aborts hard on WSLg ("xcb_xlib_threads_sequence_lost"), while native Linux
    // usually survived by luck. Serialize every X-touching window operation
    // (create / pollEvent / draw / display / close), across the UI thread and
    // all pop-up threads, on this single process-wide lock.
    inline std::mutex &gWindowXLock()
    {
        static std::mutex lock;
        return (lock);
    }

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

            if (thread.get_id() == std::this_thread::get_id())
            {
                if (thread.joinable())
                    thread.detach();
                return;
            }

            if (thread.joinable())
                thread.join();
        }
        
        void drawTitle(const std::string &text, LayoutPen layout)
        {
            sf::Text title;
            title.setFont(*font);
            title.setString(text);
            title.setCharacterSize(20);
            title.setFillColor(theme::TEXT_DIM);
            title.setPosition({layout.x, layout.y});
            window.draw(title);
        }

        void drawText(const std::string &text, LayoutPen layout)
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
            {
                std::lock_guard<std::mutex> guard(gWindowXLock());
                this->window.create({windowWidth, windowHeight}, windowTitle, this->_windowStyle);
            }

            while (this->running)
            {
                {
                    // Hold the shared X lock only around the actual X/GL work, so
                    // the main UI window keeps rendering between our frames.
                    std::lock_guard<std::mutex> guard(gWindowXLock());
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

                // Pace the pop-up (~60 FPS) with the X lock released, so it never
                // starves the main window's own rendering.
                std::this_thread::sleep_for(std::chrono::milliseconds(16));
            }
            {
                std::lock_guard<std::mutex> guard(gWindowXLock());
                window.close();
            }
        }
        protected:
            sf::Uint32 _windowStyle = sf::Style::Default;
    };
}

#endif