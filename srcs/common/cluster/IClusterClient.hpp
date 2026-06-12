//
// Created by jazema on 5/15/26.
//

#ifndef ICLUSTERCLIENT_HPP
#define ICLUSTERCLIENT_HPP
#include <functional>

#include "ConnectionState.hpp"
#include "IPacket.hpp"
#include "ServerState.hpp"
#include "../scene/IScene.hpp"

namespace rc
{
    class IClusterClient : public IPacketHandler
    {
        public:
            enum class ClientState
            {
                FETCHING_DATA,
                RECEIVING_DATA,
                RENDERING,
                SENDING_DATA,
                IDLING,
            };
            virtual ~IClusterClient() = default;

            ConnectionState getConnectionState() const override = 0;
            void setConnectionState(ConnectionState connectionState) override = 0;
            void sendPacket(const IPacket &packet) override = 0;

            virtual void join(const std::string &address, size_t port) = 0;
            virtual void disconnect() = 0;

            virtual void setInfoLogger(std::function<void(const std::string &message)> logger) = 0;
            virtual void setErrorLogger(std::function<void(const std::string &message)> logger) = 0;

            virtual void useScene(IScene *scene) = 0;
            virtual IScene *getScene() const = 0;

            virtual ClientState getClientState() const = 0;
            virtual void setClientState(ClientState clientState) = 0;
            virtual std::string getStatus() const = 0;

            virtual std::string getAddress() const = 0;
            virtual uint16_t getPort() const = 0;

            virtual ServerRenderState getServerRenderState() const = 0;
            virtual void updateServerRenderState(ServerRenderState renderState) = 0;
    };
}

#endif
