#include <iostream>
#include <string>
#include <vector>

#include "srcs/core/Core.hpp"

#define RAYTRACER_VERSION "1.0.0"

namespace
{
    struct Options
    {
        std::string scenePath;
        bool hasScene = false;

        bool headless = false;
        std::string renderOutput;

        bool startServer = false;
        size_t serverPort = 0;

        bool joinCluster = false;
        std::string clusterAddress;
        size_t clusterPort = 0;
    };

    void printUsage(const char *program)
    {
        std::cout <<
            "Usage: " << program << " [OPTIONS] [SCENE_FILE]\n"
            "\n"
            "Render synthetic 3D scenes described by a scene configuration file.\n"
            "If no SCENE_FILE is given, a blank default scene is loaded.\n"
            "\n"
            "Options:\n"
            "  -h, --help              Show this help message and exit\n"
            "  -v, --version           Show version information and exit\n"
            "  -r, --render [NAME]     Headless mode: render the scene to a PPM file\n"
            "                          without launching the UI. Optional NAME (a .ppm\n"
            "                          file) sets the output (default: render.ppm)\n"
            "      --server [PORT]     Launch the UI and start a cluster server.\n"
            "                          Optional PORT sets the listening port (default: auto)\n"
            "      --connect IP PORT   Launch the UI and join the cluster server at IP:PORT\n"
            "\n"
            "Examples:\n"
            "  " << program << " scenes/demo.cfg\n"
            "  " << program << " -r output.ppm scenes/demo.cfg\n"
            "  " << program << " --server 8080 scenes/demo.cfg\n"
            "  " << program << " --connect 127.0.0.1 8080 scenes/demo.cfg\n";
    }

    bool isFlag(const std::string &arg)
    {
        return (!arg.empty() && arg[0] == '-');
    }

    bool endsWithPpm(const std::string &arg)
    {
        return (arg.size() >= 4 && arg.compare(arg.size() - 4, 4, ".ppm") == 0);
    }

    bool parsePort(const std::string &arg, size_t &out)
    {
        try
        {
            size_t consumed = 0;
            unsigned long value = std::stoul(arg, &consumed);
            if (consumed != arg.size() || value > 65535)
                return (false);
            out = static_cast<size_t>(value);
            return (true);
        }
        catch (const std::exception &)
        {
            return (false);
        }
    }

    // Returns true on success, false on a parsing error (message already printed).
    // 'exitEarly' is set when --help / --version handled everything.
    bool parseArgs(const std::vector<std::string> &args, const char *program, Options &opts, bool &exitEarly)
    {
        for (size_t i = 0; i < args.size(); ++i)
        {
            const std::string &arg = args[i];

            if (arg == "-h" || arg == "--help")
            {
                printUsage(program);
                exitEarly = true;
                return (true);
            }
            if (arg == "-v" || arg == "--version")
            {
                std::cout << "raytracer version " << RAYTRACER_VERSION << std::endl;
                exitEarly = true;
                return (true);
            }
            if (arg == "-r" || arg == "--render")
            {
                opts.headless = true;
                // Consume the next token as the output name only when it is a .ppm
                // file, so that "-r scene.cfg" still treats scene.cfg as the scene.
                if (i + 1 < args.size() && !isFlag(args[i + 1]) && endsWithPpm(args[i + 1]))
                    opts.renderOutput = args[++i];
                continue;
            }
            if (arg == "--server")
            {
                opts.startServer = true;
                if (i + 1 < args.size() && !isFlag(args[i + 1]) && parsePort(args[i + 1], opts.serverPort))
                    ++i;
                continue;
            }
            if (arg == "--connect")
            {
                opts.joinCluster = true;
                if (i + 2 >= args.size() || isFlag(args[i + 1]) || !parsePort(args[i + 2], opts.clusterPort))
                {
                    std::cerr << "Error: --connect requires an IP address and a PORT." << std::endl;
                    return (false);
                }
                opts.clusterAddress = args[i + 1];
                i += 2;
                continue;
            }
            if (isFlag(arg))
            {
                std::cerr << "Error: unknown option '" << arg << "'. Use --help for usage." << std::endl;
                return (false);
            }
            if (opts.hasScene)
            {
                std::cerr << "Error: unexpected extra argument '" << arg << "'." << std::endl;
                return (false);
            }
            opts.scenePath = arg;
            opts.hasScene = true;
        }
        if (opts.headless && (opts.startServer || opts.joinCluster))
        {
            std::cerr << "Error: --render (headless) cannot be combined with --server or --connect." << std::endl;
            return (false);
        }
        if (opts.startServer && opts.joinCluster)
        {
            std::cerr << "Error: --server and --connect cannot be used together." << std::endl;
            return (false);
        }
        return (true);
    }
}

int main(int argc, char **argv)
{
    Options opts;
    bool exitEarly = false;
    std::vector<std::string> args(argv + 1, argv + argc);

    if (!parseArgs(args, argv[0], opts, exitEarly))
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
