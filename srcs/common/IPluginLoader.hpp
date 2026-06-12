//
// Created by jazema on 4/22/26.
//

#ifndef IPLUGINLOADER_HPP
#define IPLUGINLOADER_HPP
#include <string>
#include <vector>

#include "IPlugin.hpp"

namespace rc
{
    class IPluginLoader
    {
        public:
            struct PluginHandle
            {
                void* handle;
                IPlugin* instance;
                void (*destroy)(IPlugin*);
            };

            virtual ~IPluginLoader() = default;

            virtual bool load(const std::string &path) = 0;
            virtual void unloadAll() = 0;
            virtual const std::vector<PluginHandle> &getPlugins() const = 0;
            virtual const std::vector<PluginHandle> getPlugins(PluginType pluginType) const = 0;
    };
}

#endif
