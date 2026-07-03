#include <iostream>

#include "srcs/core/Core.hpp"

int main(int argc, char **argv)
{
    if (argc == 2 && std::string(argv[1]) == "--help")
    {
        std::cout << "Usage: " << argv[0] << " <SCENE_FILE>" << std::endl;
        std::cout << "  SCENE_FILE: scene configuration" << std::endl;
        return (0);
    }
    rc::Core core;

    try
    {
        core.loadPlugins();
    }
    catch (std::exception &e)
    {
        std::cerr << "Error loading plugins: " << e.what() << std::endl;
        return (84);
    }
    try
    {
        core.loadRenderers();
    }
    catch (std::exception &e)
    {
        std::cerr << "Error loading renderers: " << e.what() << std::endl;
        return (84);
    }
    try
    {
        if (argc >= 2)
            core.loadScene(argv[1]);
        else
            core.loadBlankScene();
    }
    catch (std::exception &e)
    {
        std::cerr << "Error loading scene: " << argv[1] << ": " << e.what() << std::endl;
        return (84);
    }
    try
    {
        core.loadUserInterface();
    }
    catch (std::exception &e)
    {
        std::cerr << "Error loading user interface: " << e.what() << std::endl;
        return (84);
    }
    try
    {
        core.startRendering();
    }
    catch (std::exception &e)
    {
        std::cerr << "Error rendering scene: " << e.what() << std::endl;
        core.unloadUserInterface();
        core.unloadScene();
        return (84);
    }
    return (0);
}
