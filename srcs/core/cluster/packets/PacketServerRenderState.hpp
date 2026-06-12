//
// Created by jazema on 5/17/26.
//

#ifndef RAYTRACER_PACKETSERVERRENDERSTATE_HPP
#define RAYTRACER_PACKETSERVERRENDERSTATE_HPP
#include "../../../common/cluster/IPacket.hpp"

namespace rc
{
    class PacketServerRenderState: public IPacket
    {
        public:
            uint16_t state;

            void deserialize(std::vector<uint8_t> data) override;
            std::vector<uint8_t> serialize() const override;

            void handle(IPacketHandler &handler) override;

            PacketID getId() const override;
    };
}

#endif
