//
// Created by jazema on 5/13/26.
//

#ifndef IPACKETHANDLER_HPP
#define IPACKETHANDLER_HPP
#include <string>

namespace rc
{
    class IPacket;

    enum class ConnectionState;

    class IPacketHandler
    {
        public:
            virtual ~IPacketHandler() = default;

            virtual void sendPacket(const IPacket &packet) = 0;

            virtual ConnectionState getConnectionState() const = 0;
            virtual void setConnectionState(ConnectionState connectionState) = 0;

            virtual void log(const std::string &message) = 0;
    };
}

#endif
