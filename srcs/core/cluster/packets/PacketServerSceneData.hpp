//
// Created by jazema on 5/16/26.
//

#ifndef PACKETSERVERSCENEDATA_HPP
#define PACKETSERVERSCENEDATA_HPP

#include <string>

#include "../../../common/cluster/IPacket.hpp"

namespace rc
{
    class PacketServerSceneData : public IPacket
    {
        public:
            std::string sceneData;

            void deserialize(std::vector<uint8_t> data) override;
            std::vector<uint8_t> serialize() const override;

            void handle(IPacketHandler &handler) override;

            PacketID getId() const override;
    };
}

#endif
