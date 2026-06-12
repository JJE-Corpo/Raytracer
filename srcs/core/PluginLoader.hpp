//
// Created by jazema on 4/21/26.
//

#ifndef PLUGINLOADER_HPP
#define PLUGINLOADER_HPP
#include <string>
#include <vector>

#include "../common/IPlugin.hpp"
#include "../common/IPluginLoader.hpp"

namespace rc
{
    class IPlugin;

    class PluginLoader : public IPluginLoader
    {
        public:
            using CreateFunc = IPlugin* (*)();
            using DestroyFunc = void (*)(IPlugin*);

            ~PluginLoader();

            bool load(const std::string &path) override;
            void unloadAll() override;
            const std::vector<PluginHandle> &getPlugins() const override;
            const std::vector<PluginHandle> getPlugins(PluginType pluginType) const override;
        private:
            std::vector<PluginHandle> _plugins;
    };
}

#endif
