

#include <iostream>
#include <vector>

#define RAYTRACER_VERSION "1.0.0"

namespace rc
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

        const char *program = nullptr;

        void setProgramName(const char *name)
        {
            program = name;
        }

        void printUsage()
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
                "  -r, --render [NAME]     Headless mode: render the scene to a PNG file\n"
                "                          without launching the UI. Optional NAME (a .png\n"
                "                          file) sets the output (default: render.png)\n"
                "      --server [PORT]     Launch the UI and start a cluster server.\n"
                "                          Optional PORT sets the listening port (default: auto)\n"
                "      --connect IP PORT   Launch the UI and join the cluster server at IP:PORT\n"
                "\n"
                "Examples:\n"
                "  " << program << " scenes/demo.cfg\n"
                "  " << program << " -r output.png scenes/demo.cfg\n"
                "  " << program << " --server 8080 scenes/demo.cfg\n"
                "  " << program << " --connect 127.0.0.1 8080 scenes/demo.cfg\n";
        }

        bool isFlag(const std::string &arg)
        {
            return (!arg.empty() && arg[0] == '-');
        }

        bool endsWithPng(const std::string &arg)
        {
            return (arg.size() >= 4 && arg.compare(arg.size() - 4, 4, ".png") == 0);
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

        bool parseArgs(const std::vector<std::string> &args, bool &exitEarly)
        {
            for (size_t i = 0; i < args.size(); ++i)
            {
                const std::string &arg = args[i];

                if (arg == "-h" || arg == "--help")
                {
                    printUsage();
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
                    headless = true;
                    if (i + 1 < args.size() && !isFlag(args[i + 1]) && endsWithPng(args[i + 1]))
                        renderOutput = args[++i];
                    continue;
                }
                if (arg == "--server")
                {
                    startServer = true;
                    if (i + 1 < args.size() && !isFlag(args[i + 1]) && parsePort(args[i + 1], serverPort))
                        ++i;
                    continue;
                }
                if (arg == "--connect")
                {
                    joinCluster = true;
                    if (i + 2 >= args.size() || isFlag(args[i + 1]) || !parsePort(args[i + 2], clusterPort))
                    {
                        std::cerr << "Error: --connect requires an IP address and a PORT." << std::endl;
                        return (false);
                    }
                    clusterAddress = args[i + 1];
                    i += 2;
                    continue;
                }
                if (isFlag(arg))
                {
                    std::cerr << "Error: unknown option '" << arg << "'. Use --help for usage." << std::endl;
                    return (false);
                }
                if (hasScene)
                {
                    std::cerr << "Error: unexpected extra argument '" << arg << "'." << std::endl;
                    return (false);
                }
                scenePath = arg;
                hasScene = true;
            }
            if (headless && (startServer || joinCluster))
            {
                std::cerr << "Error: --render (headless) cannot be combined with --server or --connect." << std::endl;
                return (false);
            }
            if (startServer && joinCluster)
            {
                std::cerr << "Error: --server and --connect cannot be used together." << std::endl;
                return (false);
            }
            return (true);
        }
    };

    
}