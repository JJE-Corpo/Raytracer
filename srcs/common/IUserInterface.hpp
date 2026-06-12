//
// Created by jazema on 4/22/26.
//

#ifndef IUSERINTERFACE_HPP
#define IUSERINTERFACE_HPP
#include "ICoreAccess.hpp"
#include "IPlugin.hpp"

namespace rc
{
    class IUserInterface : public IPlugin
    {
        public:
            virtual ~IUserInterface() = default;

            virtual void create(ICoreAccess &core_access) = 0;
            virtual void destroy() = 0;

            PluginType getType() const override = 0;
    };
}

#endif
