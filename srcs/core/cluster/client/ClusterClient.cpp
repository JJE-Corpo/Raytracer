//
// Created by jazema on 5/13/26.
//

#include "ClusterClient.hpp"

#include <iostream>
#include <libconfig.h++>
#include <stdexcept>
#include <thread>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/poll.h>
#include <sys/socket.h>
#include <fcntl.h>

#include "../../../common/cluster/ByteBuffer.hpp"
#include "../../../common/render/RenderKernel.hpp"
#include "../../scene/SceneParser.hpp"
#include "../packets/PacketClientJoinRequest.hpp"
#include "../packets/PacketClientTileData.hpp"
#include "../packets/PacketFactory.hpp"

namespace rc
{
    ClusterClient::ClusterClient() : _socketFd(-1), _connectionState(ConnectionState::DISCONNECTED), _scene(nullptr)
    {

    }

    ClusterClient::~ClusterClient()
    {
        this->ClusterClient::disconnect();
    }

    void ClusterClient::join(const std::string &address, const size_t port)
    {
        if (this->_running.load())
            throw std::runtime_error("Client already connected");
        {
            std::lock_guard lock(this->_stateMutex);
            this->_address = address;
            this->_port = port;
        }
        this->_socketFd = ::socket(AF_INET, SOCK_STREAM, 0);
        if (this->_socketFd == -1)
            throw std::runtime_error("Error creating client socket");
        std::cout << "Clustering -> Trying to join " << address << ":" << port << std::endl;
        sockaddr_in addr{};
        addr.sin_family = AF_INET;
        addr.sin_port = htons(port);
        if (::inet_pton(AF_INET, address.c_str(), &addr.sin_addr) <= 0)
        {
            ::close(this->_socketFd);
            this->_socketFd = -1;
            throw std::runtime_error("Invalid address");
        }
        if (::connect(this->_socketFd, (sockaddr *) &addr, sizeof(addr)) < 0)
        {
            ::close(this->_socketFd);
            this->_socketFd = -1;
            throw std::runtime_error("Error connecting to server");
        }
        {
            std::lock_guard lock(this->_stateMutex);
            this->_connectionState = ConnectionState::PENDING;
        }
        ::fcntl(this->_socketFd, F_SETFL, O_NONBLOCK);
        this->sendPacket(PacketClientJoinRequest());
        this->startThread();
        this->startRenderThread();
    }

    void ClusterClient::disconnect()
    {
        this->_running.store(false);
        this->stopRenderThread();
        int socketFd = -1;

        {
            std::lock_guard lock(this->_stateMutex);
            socketFd = this->_socketFd;
        }

        if (socketFd != -1)
        {
            ::shutdown(socketFd, SHUT_RDWR);
            ::close(socketFd);
        }

        {
            std::lock_guard lock(this->_stateMutex);
            this->_socketFd = -1;
            this->_connectionState = ConnectionState::DISCONNECTED;
            this->_clientState = ClientState::IDLING;
        }

        if (this->_thread.joinable() && this->_thread.get_id() != std::this_thread::get_id())
            this->_thread.join();
    }

    void ClusterClient::run()
    {
        pollfd fd{};
        int socketFd = -1;

        {
            std::lock_guard lock(this->_stateMutex);
            socketFd = this->_socketFd;
        }

        if (socketFd == -1)
        {
            this->_running.store(false);
            return;
        }

        fd = {socketFd, POLLIN | POLLOUT, 0};
        while (this->_running.load())
        {
            if (::poll(&fd, 1, 200) < 0)
                break;
            if (!this->_running.load())
                break;
            if (fd.revents & (POLLERR | POLLHUP | POLLNVAL))
                break;
            if (fd.revents & POLLIN && !this->handleRead())
                break;
            if (fd.revents & POLLOUT && !this->handleWrite())
                break;
        }

        this->_running.store(false);
        this->setConnectionState(ConnectionState::DISCONNECTED);
    }

    void ClusterClient::startThread()
    {
        if (this->_thread.joinable())
            this->_thread.join();
        this->_running.store(true);
        this->_thread = std::thread([this]
        {
            this->run();
        });
    }

    void ClusterClient::startRenderThread()
    {
        if (this->_renderThread.joinable())
            this->_renderThread.join();
        this->_renderRunning.store(true);
        this->_renderThread = std::thread([this]
        {
            this->renderLoop();
        });
    }

    void ClusterClient::stopRenderThread()
    {
        this->_renderRunning.store(false);
        this->_renderCv.notify_all();
        if (this->_renderThread.joinable() && this->_renderThread.get_id() != std::this_thread::get_id())
            this->_renderThread.join();
    }

    void ClusterClient::sendPacket(const IPacket &packet)
    {
        ByteBuffer buffer;
        std::vector<uint8_t> payload;

        if (this->_socketFd == -1)
            throw (std::runtime_error("Socket not connected"));
        payload = packet.serialize();
        buffer.write(static_cast<uint16_t>(packet.getId()));
        buffer.write(static_cast<uint32_t>(payload.size()));
        buffer.data.insert(buffer.data.end(), payload.begin(), payload.end());
        std::lock_guard lock(this->_writeMutex);
        this->_writeBuffer.data.insert(this->_writeBuffer.data.end(), buffer.data.begin(), buffer.data.end());
    }


    bool ClusterClient::handleRead()
    {
        uint8_t buffer[1024];
        ssize_t bytes;

        bytes = ::read(this->_socketFd, buffer, sizeof(buffer));
        if (bytes <= 0)
            return (false);
        this->_readBuffer.data.insert(this->_readBuffer.data.end(), buffer, buffer + bytes);
        while (this->_readBuffer.data.size() >= 6)
        {
            uint16_t rawId;
            uint32_t rawSize;

            std::memcpy(&rawId, this->_readBuffer.data.data(), sizeof(rawId));
            std::memcpy(&rawSize, this->_readBuffer.data.data() + 2, sizeof(rawSize));

            uint16_t packetId = ntohs(rawId);
            uint32_t payloadSize = ntohl(rawSize);

            if (this->_readBuffer.data.size() < 6 + payloadSize)
                break;

            std::vector payload(this->_readBuffer.data.begin() + 6, this->_readBuffer.data.begin() + 6 + payloadSize);

            this->_readBuffer.data.erase(this->_readBuffer.data.begin(), this->_readBuffer.data.begin() + 6 + payloadSize);

            auto packet = PacketFactory::createPacket(static_cast<PacketID>(packetId), payload);
            if (packet)
                packet->handle(*this);
        }
        return (true);
    }

    bool ClusterClient::handleWrite()
    {
        ssize_t bytes;

        std::lock_guard lock(this->_writeMutex);
        while (!this->_writeBuffer.data.empty())
        {
            bytes = ::write(this->_socketFd, this->_writeBuffer.data.data(), this->_writeBuffer.data.size());
            if (bytes == -1)
                return (false);
            if (bytes == 0)
                return (true);
            this->_writeBuffer.data.erase(this->_writeBuffer.data.begin(), this->_writeBuffer.data.begin() + bytes);
        }
        return (true);
    }

    std::string ClusterClient::getAddress() const
    {
        std::lock_guard lock(this->_stateMutex);
        return (this->_address);
    }

    uint16_t ClusterClient::getPort() const
    {
        std::lock_guard lock(this->_stateMutex);
        return (this->_port);
    }

    IClusterClient::ClientState ClusterClient::getClientState() const
    {
        std::lock_guard lock(this->_stateMutex);
        return (this->_clientState);
    }

    void ClusterClient::setClientState(const ClientState clientState)
    {
        std::lock_guard lock(this->_stateMutex);
        this->_clientState = clientState;
    }

    std::string ClusterClient::getStatus() const
    {
        ClientState clientState;
        ConnectionState connectionState;

        {
            std::lock_guard lock(this->_stateMutex);
            clientState = this->_clientState;
            connectionState = this->_connectionState;
        }

        if (connectionState == ConnectionState::DISCONNECTED)
            return ("Disconnected");
        if (connectionState == ConnectionState::PENDING)
            return ("Connecting: Waiting for server..");
        if (connectionState == ConnectionState::REFUSED)
            return ("Connection refused");
        //todo si on est la cest quon est bien co
        if (clientState == ClientState::FETCHING_DATA)
            return ("Fetching data");
        if (clientState == ClientState::RECEIVING_DATA)
            return ("Receiving data");
        if (clientState == ClientState::RENDERING)
            return ("Rendering");
        return ("Idling");
    }

    void ClusterClient::useScene(IScene *scene)
    {
        delete (this->_scene);
        this->_scene = scene;
        log("Building scene BVH !");
        this->_scene->buildBvh();
        log("Scene loaded ! (" + std::to_string(this->_scene->getPrimitives().size()) + " primitives, " + std::to_string(this->_scene->getLights().size()) + " lights).");
        _infoToast("Scene received from server !");
    }
    
    IScene *ClusterClient::getScene() const
    {
        std::lock_guard lock(this->_stateMutex);
        return this->_scene;
    }

    void ClusterClient::enqueueRenderJob(const RenderJob &job)
    {
        {
            std::lock_guard lock(this->_renderMutex);
            this->_renderQueue.push_back(job);
        }
        this->_renderCv.notify_one();
    }

    void ClusterClient::cancelRenderJobs()
    {
        {
            std::lock_guard lock(this->_renderMutex);
            this->_renderQueue.clear();
        }
        this->setClientState(ClientState::IDLING);
        this->_renderCv.notify_all();
        this->_infoToast("Server cancelled launched redering jobs");
    }

    void ClusterClient::renderLoop()
    {
        while (this->_renderRunning.load())
        {
            if (this->getClientState() != ClientState::IDLING)
                continue;
            RenderJob job;
            {
                std::unique_lock lock(this->_renderMutex);
                this->_renderCv.wait(lock, [&]
                {
                    return (!this->_renderRunning.load() || !this->_renderQueue.empty());
                });
                if (!this->_renderRunning.load())
                    break;
                if (this->_renderQueue.empty())
                    continue;
                job = this->_renderQueue.front();
                this->_renderQueue.pop_front();
            }

            if (!this->_scene)
            {
                this->_renderQueue.push_front(job);
                continue;
            }

            this->setClientState(ClientState::RENDERING);

            std::vector<ColorF> pixels;
            render_kernel::render_tile_sample(*this->_scene, this->_scene->getCamera(),
                job.start_x, job.start_y, job.end_x, job.end_y, pixels);

            this->setClientState(ClientState::SENDING_DATA);

            PacketClientTileData response;
            response.tile_id = job.tile_id;
            response.sample = job.sample;
            response.start_x = static_cast<uint32_t>(job.start_x);
            response.start_y = static_cast<uint32_t>(job.start_y);
            response.end_x = static_cast<uint32_t>(job.end_x);
            response.end_y = static_cast<uint32_t>(job.end_y);
            response.pixels = std::move(pixels);
            try
            {
                this->sendPacket(response);
            }
            catch (const std::exception &e)
            {
                this->log(std::string("Failed to send tile: ") + e.what());
            }

            this->setClientState(ClientState::IDLING);
        }
    }

    ConnectionState ClusterClient::getConnectionState() const
    {
        std::lock_guard lock(this->_stateMutex);
        return (this->_connectionState);
    }

    void ClusterClient::setConnectionState(const ConnectionState connectionState)
    {
        std::lock_guard lock(this->_stateMutex);
        this->_connectionState = connectionState;
    }

    void ClusterClient::updateServerRenderState(ServerRenderState renderState)
    {
        std::lock_guard lock(this->_stateMutex);
        this->_serverState = renderState;
    }

    ServerRenderState ClusterClient::getServerRenderState() const
    {
        std::lock_guard lock(this->_stateMutex);
        return (this->_serverState);
    }

    void ClusterClient::setInfoLogger(std::function<void(const std::string &message)> infoToast)
    {
        this->_infoToast = infoToast;
    }

    void ClusterClient::setErrorLogger(std::function<void(const std::string &message)> infoToast)
    {
        this->_errorToast = infoToast;
    }

    void ClusterClient::log(const std::string &message)
    {
        std::cout << "Clustering -> " << message << std::endl;
    }
}
