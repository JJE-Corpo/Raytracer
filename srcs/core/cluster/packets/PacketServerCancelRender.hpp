//
// Created by jazema on 5/16/26.
//

#ifndef PACKETSERVERCANCELRENDER_HPP
#define PACKETSERVERCANCELRENDER_HPP

#include "../../../common/cluster/IPacket.hpp"

namespace rc
{
    class PacketServerCancelRender : public IPacket
    {
        public:
            void deserialize(std::vector<uint8_t> data) override;
            std::vector<uint8_t> serialize() const override;

            void handle(IPacketHandler &handler) override;

            PacketID getId() const override;
    };
}

#endif
