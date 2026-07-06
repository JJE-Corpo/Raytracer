//
// Created by jazema on 5/13/26.
//

#include "ClusterServer.hpp"

#include <iostream>
#include <unistd.h>
#include <vector>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/poll.h>

#include "Connection.hpp"
#include "../../../common/cluster/ServerState.hpp"
#include "../../scene/SceneRegister.hpp"
#include "../packets/PacketClientTileData.hpp"
#include "../packets/PacketServerCancelRender.hpp"
#include "../packets/PacketServerRenderRequest.hpp"
#include "../packets/PacketServerRenderState.hpp"
#include "../packets/PacketServerSceneData.hpp"

namespace rc
{
    ClusterServer::ClusterServer(IScene *scene, uint16_t port): _serverFd(-1), _configuredPort(port), _scene(scene)
    {
    }

    ClusterServer::~ClusterServer()
    {
        this->ClusterServer::stop();
    }

    void ClusterServer::threadEndpoint()
    {
        this->_serverFd = ::socket(AF_INET, SOCK_STREAM, 0);
        if (this->_serverFd == -1)
        {
            std::cerr << "Could not create cluster server" << std::endl;
            return;
        }
        sockaddr_in serverAddr{};
        serverAddr.sin_addr.s_addr = INADDR_ANY;
        serverAddr.sin_port = htons(this->_configuredPort);
        serverAddr.sin_family = AF_INET;
        if (::bind(this->_serverFd, (sockaddr *)&serverAddr, sizeof(serverAddr)) == -1)
        {
            std::cerr << "Could not bind cluster socket on a port" << std::endl;
            ::close(this->_serverFd);
            this->_serverFd = -1;
            return;
        }
        if (::listen(this->_serverFd, SOMAXCONN) == -1)
        {
            std::cerr << "Could not listen on cluster socket" << std::endl;
            ::close(this->_serverFd);
            this->_serverFd = -1;
            return;
        }
        sockaddr_in clusterAddr{};
        socklen_t addressLen = sizeof(clusterAddr);
        getsockname(this->_serverFd, (sockaddr *) &clusterAddr, &addressLen);
        this->_serverPort = ntohs(clusterAddr.sin_port);
        std::cout << "Clustering -> Cluster server started ! Listening on port " << this->_serverPort << std::endl;
        this->_running = true;
        while (this->_running)
        {
            this->handleClients();
        }
    }

    void ClusterServer::handleClientDisconnect(const int connectionFd)
    {
        // ClusterRenderCoordinator *coordinator = nullptr;
        // {
        // std::lock_guard lock(this->_serverMutex);
        // coordinator = this->_renderCoordinator;
        // }
        // if (coordinator)
        // coordinator->markClientDisconnected(this->_connections[i]->getFd());
        (void)connectionFd;
    }

    void ClusterServer::handleClients()
    {
        std::vector<pollfd> pollfds;
        size_t              i;

        if (!this->_running)
            return;
        {
            std::lock_guard lock(this->_connectionsMutex);
            pollfds.push_back(pollfd{this->_serverFd, POLLIN, 0});
            for (auto &connectionPtr: this->_connections)
            {
                pollfds.push_back(pollfd{connectionPtr->getFd(), POLLIN | POLLOUT, 0});
            }
        }
        // poll() blocks up to 200ms; keep it out of the lock so broadcasts from
        // the render thread are not stalled behind it.
        if (::poll(pollfds.data(), pollfds.size(), 200) == -1)
            return;
        if (pollfds.empty())
            return;
        if (!this->_running)
            return;
        {
            std::lock_guard lock(this->_connectionsMutex);
            if (pollfds[0].revents & POLLIN)
            {
                auto newConnection = std::make_unique<Connection>(this);
                try
                {
                    newConnection->open(this->_serverFd);
                    newConnection->log("Is pending. Waiting for connection packet..");
                    this->_connections.push_back(std::move(newConnection));
                } catch (std::exception &) {}
            }
            i = 0;
            while (i < this->_connections.size())
            {
                size_t pollIndex = i + 1;
                if (pollIndex >= pollfds.size())
                    break;
                if (pollfds[pollIndex].revents & POLLIN && !this->_connections[i]->handleRead())
                {
                    handleClientDisconnect(this->_connections[i]->getFd());
                    this->_connections.erase(this->_connections.begin() + i);
                    pollfds.erase(pollfds.begin() + (pollIndex));
                    continue;
                }
                if (pollfds[pollIndex].revents & POLLOUT && !this->_connections[i]->handleWrite())
                {
                    handleClientDisconnect(this->_connections[i]->getFd());
                    this->_connections.erase(this->_connections.begin() + i);
                    pollfds.erase(pollfds.begin() + (pollIndex));
                    continue;
                }
                if (pollfds[pollIndex].revents & (POLLHUP | POLLERR))
                {
                    handleClientDisconnect(this->_connections[i]->getFd());
                    this->_connections.erase(this->_connections.begin() + i);
                    pollfds.erase(pollfds.begin() + (pollIndex));
                    continue;
                }
                i++;
            }
        }
        this->dispatchRenderRequests();
    }

    void ClusterServer::dispatchRenderRequests()
    {
        ClusterRenderCoordinator *coordinator = nullptr;
        {
            std::lock_guard lock(this->_serverMutex);
            coordinator = this->_renderCoordinator;
        }

        if (!coordinator || !coordinator->isActive())
            return;

        // Reclaim tiles handed to clients that never returned them (slow or
        // dead) so the sample can still complete and the render never hangs.
        coordinator->requeueTimedOut(this->_tileTimeout);

        std::lock_guard lock(this->_connectionsMutex);
        for (auto &connectionPtr : this->_connections)
        {
            if (!connectionPtr)
                continue;
            if (connectionPtr->getConnectionState() != ConnectionState::CONNECTED)
                continue;

            ClusterRenderCoordinator::TileJob job;

            if (!coordinator->popRemoteJob(job))
                continue;

            PacketServerRenderRequest request;
            request.tile_id = job.tile_id;
            request.sample = job.sample;
            request.start_x = static_cast<uint32_t>(job.start_x);
            request.start_y = static_cast<uint32_t>(job.start_y);
            request.end_x = static_cast<uint32_t>(job.end_x);
            request.end_y = static_cast<uint32_t>(job.end_y);
            connectionPtr->sendPacket(request);
        }
    }

    void ClusterServer::start()
    {
        this->_serverThread = std::thread(&ClusterServer::threadEndpoint, this);
    }

    void ClusterServer::stop()
    {
        this->_running = false;
        int serverFd = this->_serverFd.load();

        if (serverFd != -1)
        {
            ::close(serverFd);
            this->_serverFd = -1;
        }

        if (this->_serverThread.joinable() && this->_serverThread.get_id() != std::this_thread::get_id())
            this->_serverThread.join();

        std::lock_guard lock(this->_connectionsMutex);
        this->_connections.clear();
    }

    uint16_t ClusterServer::getPort() const
    {
        return (this->_serverPort);
    }

    IScene *ClusterServer::getScene()
    {
        return (this->_scene);
    }

    void ClusterServer::setRenderCoordinator(ClusterRenderCoordinator *coordinator, IClusterTileSink *sink)
    {
        std::lock_guard lock(this->_serverMutex);
        this->_renderCoordinator = coordinator;
        this->_tileSink = sink;
    }

    void ClusterServer::clearRenderCoordinator()
    {
        std::lock_guard lock(this->_serverMutex);
        this->_renderCoordinator = nullptr;
        this->_tileSink = nullptr;
    }

    void ClusterServer::handleClientTileData(int connection_fd, const PacketClientTileData &packet)
    {
        ClusterRenderCoordinator *coordinator = nullptr;
        IClusterTileSink *sink = nullptr;

        {
            std::lock_guard lock(this->_serverMutex);
            coordinator = this->_renderCoordinator;
            sink = this->_tileSink;
        }

        if (!coordinator || !sink)
            return;

        ClusterRenderCoordinator::TileJob job;
        job.tile_id = packet.tile_id;
        job.sample = packet.sample;
        job.start_x = static_cast<int>(packet.start_x);
        job.start_y = static_cast<int>(packet.start_y);
        job.end_x = static_cast<int>(packet.end_x);
        job.end_y = static_cast<int>(packet.end_y);
        (void)connection_fd;

        // Only accumulate the pixels if this tile actually completes the current
        // sample for the first time (drops stale / duplicate tile submissions).
        if (coordinator->markComplete(job))
            sink->applyTileSample(job, packet.pixels);
    }

    void ClusterServer::broadcastCancelRender()
    {
        PacketServerCancelRender cancel;
        std::lock_guard lock(this->_connectionsMutex);
        for (auto &connectionPtr : this->_connections)
        {
            if (!connectionPtr)
                continue;
            if (connectionPtr->getConnectionState() != ConnectionState::CONNECTED)
                continue;
            connectionPtr->sendPacket(cancel);
        }
    }

    void ClusterServer::broadcastScene()
    {
        PacketServerSceneData data;

        data.sceneData = SceneRegister().toString(this->_scene);
        std::lock_guard lock(this->_connectionsMutex);
        for (auto &connectionPtr : this->_connections)
        {
            if (!connectionPtr)
                continue;
            if (connectionPtr->getConnectionState() != ConnectionState::CONNECTED)
                continue;
            connectionPtr->sendPacket(data);
        }
    }

    void ClusterServer::broadcastServerState(ServerRenderState state)
    {
        PacketServerRenderState packet;

        packet.state = static_cast<uint16_t>(state);
        std::lock_guard lock(this->_connectionsMutex);
        for (auto &connectionPtr : this->_connections)
        {
            if (!connectionPtr)
                continue;
            if (connectionPtr->getConnectionState() != ConnectionState::CONNECTED)
                continue;
            connectionPtr->sendPacket(packet);
        }
    }
}
