//
// Created by jazema on 5/16/26.
//

#include "Connection.hpp"

#include <iostream>
#include <stdexcept>
#include <unistd.h>
#include <vector>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>

#include "../../../common/cluster/ByteBuffer.hpp"
#include "../../../common/cluster/IPacket.hpp"
#include "../packets/PacketFactory.hpp"

namespace rc
{
    Connection::Connection(IClusterServer *server)
    {
        this->_serverPtr = server;
    }

    Connection::~Connection()
    {
        if (this->_connectionFd != -1)
        {
            ::close(this->_connectionFd);
            this->_connectionFd = -1;
        }
    }

    void Connection::open(int socket)
    {
        sockaddr_in peer{};
        socklen_t peerLen = sizeof(peer);

        this->_connectionFd = accept(socket, reinterpret_cast<sockaddr *>(&peer), &peerLen);
        if (this->_connectionFd < 0)
            throw std::runtime_error("Accept failed");

        char host[INET_ADDRSTRLEN] = {0};
        if (::inet_ntop(AF_INET, &peer.sin_addr, host, sizeof(host)) != nullptr)
            this->_address = std::string(host) + ":" + std::to_string(ntohs(peer.sin_port));
    }

    void Connection::sendPacket(const IPacket &packet)
    {
        ByteBuffer           buffer;
        std::vector<uint8_t> payload;

        if (this->_connectionFd == -1)
            throw (std::runtime_error("Socket not connected"));
        payload = packet.serialize();
        buffer.write(static_cast<uint16_t>(packet.getId()));
        buffer.write(static_cast<uint32_t>(payload.size()));
        buffer.data.insert(buffer.data.end(), payload.begin(), payload.end());
        std::lock_guard<std::mutex> lock(this->_writeMutex);
        this->_writeBuffer.data.insert(this->_writeBuffer.data.end(), buffer.data.begin(), buffer.data.end());
    }

    bool Connection::handleRead()
    {
        uint8_t tmp[1024];
        ssize_t bytes;

        bytes = ::read(this->_connectionFd, tmp, sizeof(tmp));
        if (bytes <= 0)
            return (false);
        this->_readBuffer.data.insert(this->_readBuffer.data.end(), tmp, tmp + bytes);
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

            // The framing above is length-prefixed, so a malformed payload does
            // not desync the stream: drop just this packet and keep the client.
            try
            {
                auto packet = PacketFactory::createPacket(static_cast<PacketID>(packetId), payload);
                if (packet)
                    packet->handle(*this);
            }
            catch (const std::exception &e)
            {
                this->log(std::string("Dropped malformed packet: ") + e.what());
            }
        }
        return (true);
    }

    bool Connection::handleWrite()
    {
        ssize_t bytes;

        std::lock_guard<std::mutex> lock(this->_writeMutex);
        while (!this->_writeBuffer.data.empty())
        {
            bytes = write(this->_connectionFd, this->_writeBuffer.data.data(), this->_writeBuffer.data.size());
            if (bytes == -1)
                return (false);
            if (bytes == 0)
                return (true);
            this->_writeBuffer.data.erase(this->_writeBuffer.data.begin(), this->_writeBuffer.data.begin() + bytes);
        }
        return (true);
    }

    std::string Connection::getName()
    {
        return (this->_name);
    }

    std::string Connection::getAddress() const
    {
        return (this->_address);
    }

    int Connection::getFd()
    {
        return (this->_connectionFd);
    }

    uint64_t Connection::getTilesRendered() const
    {
        return (this->_tilesRendered.load());
    }

    void Connection::incrementTilesRendered()
    {
        this->_tilesRendered.fetch_add(1);
    }

    ConnectionState Connection::getConnectionState() const
    {
        return (this->_state);
    }

    void Connection::setConnectionState(ConnectionState connectionState)
    {
        this->_state = connectionState;
    }

    IClusterServer *Connection::getServer() const
    {
        return (this->_serverPtr);
    }

    void Connection::log(const std::string &message)
    {
        std::cout << "Clustering -> Client " << this->getName() << " (" << this->getFd() << ") | " << message << std::endl;
    }
}
