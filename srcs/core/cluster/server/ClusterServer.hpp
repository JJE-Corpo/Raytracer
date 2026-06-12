//
// Created by jazema on 5/13/26.
//

#ifndef CLUSTERSERVER_HPP
#define CLUSTERSERVER_HPP
#include <atomic>
#include <chrono>
#include <mutex>
#include <thread>
#include <vector>

#include "Connection.hpp"
#include "../../../common/cluster/IClusterServer.hpp"
#include "../../../common/cluster/ServerState.hpp"
#include "../render/ClusterRenderCoordinator.hpp"
#include "../render/IClusterTileSink.hpp"

namespace rc
{
    class IScene;
    class PacketClientTileData;

    class ClusterServer : public IClusterServer
    {
        private:
            std::atomic<int> _serverFd;

            std::thread _serverThread;

            std::atomic<bool> _running = false;
            std::atomic<uint16_t> _serverPort = 0;

            std::vector<std::unique_ptr<Connection>> _connections;

            IScene *_scene;

            ClusterRenderCoordinator *_renderCoordinator = nullptr;
            IClusterTileSink *_tileSink = nullptr;
            std::chrono::milliseconds _tileTimeout{5000};

            std::mutex _serverMutex;

            void handleClientDisconnect(int connectionFd);
            void handleClients();
            void dispatchRenderRequests();

            void threadEndpoint();
        public:
            ClusterServer(IScene *scene);
            ~ClusterServer() override;

            void start() override;
            void stop() override;

            uint16_t getPort() const override;

            IScene *getScene() override;

            void setRenderCoordinator(ClusterRenderCoordinator *coordinator, IClusterTileSink *sink);
            void clearRenderCoordinator();
            void handleClientTileData(int connection_fd, const PacketClientTileData &packet);
            void broadcastCancelRender();
            void broadcastScene();
            void broadcastServerState(ServerRenderState state);
    };
}

#endif
