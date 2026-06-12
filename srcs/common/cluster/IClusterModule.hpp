//
// Created by jazema on 5/15/26.
//

#ifndef ICLUSTERMODULE_HPP
#define ICLUSTERMODULE_HPP
#include <string>

#include "IClusterClient.hpp"
#include "IClusterServer.hpp"

namespace rc
{
    enum class ClusterMode
    {
        CLIENT,
        SERVER,
        NONE
    };
    class IClusterModule
    {
        public:
            virtual ~IClusterModule() = default;

            virtual ClusterMode getClusterMode() const = 0;
            virtual IClusterServer *getClusterServer() const = 0;
            virtual IClusterClient *getClusterClient() const = 0;

            virtual void startServer(IScene *scene) = 0;
            virtual void joinCluster(const std::string &addres, size_t port) = 0;
            virtual void leaveCluster() = 0;
    };
}

#endif
