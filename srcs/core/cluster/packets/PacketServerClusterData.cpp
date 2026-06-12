//
// Created by jazema on 5/16/26.
//

#include "PacketServerClusterData.hpp"

#include "../../../common/cluster/ByteBuffer.hpp"

namespace rc
{
    void PacketServerClusterData::deserialize(std::vector<uint8_t> data)
    {
        ByteBuffer buffer;

        buffer.data = data;
        this->nb_clients = buffer.readUInt32();
    }

    std::vector<uint8_t> PacketServerClusterData::serialize() const
    {
        ByteBuffer buffer;

        buffer.write(this->nb_clients);
        return (buffer.data);
    }

    void PacketServerClusterData::handle(IPacketHandler &handler)
    {
        (void)handler;
        handler.log("Server sent cluster informations!");
        //todo handler.updateClusterData(deserialized_struct ou un truc du genre);
    }

    PacketID PacketServerClusterData::getId() const
    {
        return (PacketID::SERVER_CLUSTER_DATA);
    }
}
