//
// Created by jazema on 5/16/26.
//

#ifndef CONNECTION_HPP
#define CONNECTION_HPP
#include <mutex>
#include <string>

#include "../../../common/cluster/ByteBuffer.hpp"
#include "../../../common/cluster/ConnectionState.hpp"
#include "../../../common/cluster/IPacketHandler.hpp"

namespace rc
{
    class IPacket;
    class IClusterServer;

    class Connection : public IPacketHandler
    {
        private:
            IClusterServer *_serverPtr;

            std::string _name = "Unknown user";
            ConnectionState _state = ConnectionState::PENDING;
            int _connectionFd = -1;

            ByteBuffer _readBuffer;
            ByteBuffer _writeBuffer;
            std::mutex _writeMutex;
        public:
            Connection(IClusterServer *server);
            ~Connection();

            void open(int socket);

            void sendPacket(const IPacket &packet) override;

            bool handleWrite();
            bool handleRead();

            std::string getName();

            int getFd();

            ConnectionState getConnectionState() const override;
            void setConnectionState(ConnectionState connectionState) override;

            IClusterServer *getServer() const;

            void log(const std::string &message) override;
    };
}

#endif
