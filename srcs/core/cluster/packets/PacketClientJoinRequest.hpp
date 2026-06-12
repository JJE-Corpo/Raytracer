//
// Created by jazema on 5/14/26.
//

#ifndef PACKETCLIENTJOINREQUEST_HPP
#define PACKETCLIENTJOINREQUEST_HPP
#include "../../../common/cluster/IPacket.hpp"

namespace rc
{
    class PacketClientJoinRequest : public IPacket
    {
        public:
            std::vector<uint8_t> serialize() const override;
            void deserialize(std::vector<uint8_t> data) override;

            void handle(IPacketHandler &handler) override;

            PacketID getId() const override;
    };
}

#endif
