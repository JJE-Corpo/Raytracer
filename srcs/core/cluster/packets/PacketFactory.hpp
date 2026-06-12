//
// Created by jazema on 5/16/26.
//

#ifndef PACKETFACTORY_HPP
#define PACKETFACTORY_HPP
#include <memory>
#include <vector>

namespace rc
{
    enum class PacketID : uint8_t;
    class IPacket;

    class PacketFactory
    {
        public:
            template<typename T>
            static std::unique_ptr<IPacket> make(const std::vector<uint8_t> &payload)
            {
                auto p = std::make_unique<T>();
                p->deserialize(payload);
                return (p);
            }

            static std::unique_ptr<IPacket> createPacket(PacketID id, const std::vector<uint8_t> &payload);
    };
}

#endif
