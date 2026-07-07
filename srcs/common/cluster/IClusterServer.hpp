//
// Created by jazema on 5/15/26.
//

#ifndef ICLUSTERSERVER_HPP
#define ICLUSTERSERVER_HPP

#include <cstdint>
#include <string>
#include <vector>

#include "ConnectionState.hpp"

namespace rc
{
    class IScene;

    class IClusterServer
    {
        public:
            struct ClientInfo
            {
                std::string name;
                std::string address;
                ConnectionState state = ConnectionState::PENDING;
                uint64_t tilesRendered = 0;
            };

            virtual ~IClusterServer() = default;

            virtual void start() = 0;
            virtual void stop() = 0;

            virtual IScene *getScene() = 0;

            virtual uint16_t getPort() const = 0;

            // Thread-safe snapshot of the currently connected clients.
            virtual std::vector<ClientInfo> getClients() const = 0;
    };
}

#endif
