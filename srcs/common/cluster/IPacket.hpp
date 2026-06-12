//
// Created by jazema on 5/13/26.
//

#ifndef IPACKET_HPP
#define IPACKET_HPP
#include <cstdint>
#include <vector>

#include "IPacketHandler.hpp"
#include "PacketID.hpp"

namespace rc
{
    class IPacket
    {
        public:
            virtual ~IPacket() = default;

            virtual std::vector<uint8_t> serialize() const = 0;
            virtual void deserialize(std::vector<uint8_t> data) = 0;

            virtual void handle(IPacketHandler &handler) = 0;

            virtual PacketID getId() const = 0;
    };
}

#endif
