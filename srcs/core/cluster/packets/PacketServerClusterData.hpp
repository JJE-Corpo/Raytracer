//
// Created by jazema on 5/16/26.
//

#ifndef PACKETSERVERCLUSTERDATA_HPP
#define PACKETSERVERCLUSTERDATA_HPP
#include "../../../common/cluster/IPacket.hpp"

namespace rc
{
    class PacketServerClusterData : public IPacket
    {
        public:
            uint32_t nb_clients = 0;

            void deserialize(std::vector<uint8_t> data) override;
            std::vector<uint8_t> serialize() const override;

            void handle(IPacketHandler &handler) override;

            PacketID getId() const override;
    };
}

#endif
