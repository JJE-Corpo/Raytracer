//
// Created by jazema on 4/21/26.
//

#ifndef IPLUGIN_HPP
#define IPLUGIN_HPP

namespace rc
{
    enum class PluginType
    {
        // PRIMITIVE,
        // LIGHT,
        // SCENE_LOADER, /* Allons-nous l'enlever ? Useless a mort */
        RENDERER,
        USER_INTERFACE,
        // OPTICAL_EFFECT,
    };

    class IPlugin
    {
        public:
            virtual ~IPlugin() = default;
            virtual PluginType getType() const = 0;
    };
}

#endif
