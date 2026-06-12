//
// Created by jazema on 4/21/26.
//

#include "PluginLoader.hpp"

#include <dlfcn.h>

#include "../common/IPlugin.hpp"

namespace rc
{
    PluginLoader::~PluginLoader()
    {

    }

    bool PluginLoader::load(const std::string &path)
    {
        void *handle = dlopen(path.c_str(), RTLD_NOW);

        if (!handle)
            return (false);
        auto create = (CreateFunc)dlsym(handle, "create_plugin");
        auto destroy = (DestroyFunc)dlsym(handle, "destroy_plugin");

        if (!create || !destroy)
        {
            dlclose(handle);
            return (false);
        }

        IPlugin* plugin = create();

        this->_plugins.push_back({handle, plugin, destroy});
        return (true);
    }

    void PluginLoader::unloadAll()
    {
        for (auto& p : this->_plugins)
        {
            p.destroy(p.instance);
            dlclose(p.handle);
        }
        this->_plugins.clear();
    }

    const std::vector<PluginLoader::PluginHandle> &PluginLoader::getPlugins() const
    {
        return (this->_plugins);
    }

    const std::vector<PluginLoader::PluginHandle> PluginLoader::getPlugins(PluginType type) const
    {
        std::vector<PluginHandle> result;

        for (auto &handle : this->_plugins)
        {
            if (handle.instance->getType() == type)
                result.push_back(handle);
        }
        return (result);
    }
}
