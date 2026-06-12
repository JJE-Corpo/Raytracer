//
// Created by jazema on 5/16/26.
//

#ifndef PACKETSERVERJOINRESPONSE_HPP
#define PACKETSERVERJOINRESPONSE_HPP
#include "../../../common/cluster/IPacket.hpp"

namespace rc
{
    class PacketServerJoinResponse : public IPacket
    {
        public:
            std::vector<uint8_t> serialize() const override;
            void deserialize(std::vector<uint8_t> data) override;
            void handle(IPacketHandler &handler) override;

            PacketID getId() const override;
    };
}

#endif
