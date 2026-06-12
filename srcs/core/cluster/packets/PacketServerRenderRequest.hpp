//
// Created by jazema on 5/16/26.
//

#ifndef PACKETSERVERRENDERREQUEST_HPP
#define PACKETSERVERRENDERREQUEST_HPP

#include "../../../common/cluster/IPacket.hpp"

namespace rc
{
    class PacketServerRenderRequest : public IPacket
    {
        public:
            uint32_t tile_id = 0;
            uint32_t sample = 0;
            uint32_t start_x = 0;
            uint32_t start_y = 0;
            uint32_t end_x = 0;
            uint32_t end_y = 0;

            void deserialize(std::vector<uint8_t> data) override;
            std::vector<uint8_t> serialize() const override;

            void handle(IPacketHandler &handler) override;

            PacketID getId() const override;
    };
}

#endif
