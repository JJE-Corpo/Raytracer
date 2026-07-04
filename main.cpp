#include <iostream>
#include <string>
#include <vector>

#include "srcs/core/Core.hpp"
#include "srcs/config/Options.hpp"

#define RAYTRACER_VERSION "1.0.0"



int main(int argc, char **argv)
{
    rc::Options opts;
    bool exitEarly = false;
    std::vector<std::string> args(argv + 1, argv + argc);

    opts.setProgramName(argv[0]);

    if (!opts.parseArgs(args, exitEarly))
        return (84);
    if (exitEarly)
        return (0);

    rc::Core core;

    if (!opts.renderOutput.empty())
        core.setRenderOutput(opts.renderOutput);

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
        if (opts.hasScene)
            core.loadScene(opts.scenePath);
        else
            core.loadBlankScene();
    }
    catch (std::exception &e)
    {
        std::cerr << "Error loading scene: " << opts.scenePath << ": " << e.what() << std::endl;
        return (84);
    }
    if (!opts.headless)
    {
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
            if (opts.startServer)
                core.getClusterModule()->startServer(core.getScene(), opts.serverPort);
            else if (opts.joinCluster)
                core.getClusterModule()->joinCluster(opts.clusterAddress, opts.clusterPort);
        }
        catch (std::exception &e)
        {
            std::cerr << "Error setting up cluster: " << e.what() << std::endl;
            return (84);
        }
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
