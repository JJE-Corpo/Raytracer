//
// Created by jazema on 5/16/26.
//

#include "PacketServerCancelRender.hpp"

#include "../../../common/cluster/ByteBuffer.hpp"
#include "../../../common/cluster/PacketID.hpp"
#include "../client/ClusterClient.hpp"

namespace rc
{
    void PacketServerCancelRender::deserialize(std::vector<uint8_t> data)
    {
        ByteBuffer buffer;

        buffer.data = std::move(data);
    }

    std::vector<uint8_t> PacketServerCancelRender::serialize() const
    {
        ByteBuffer buffer;

        return buffer.data;
    }

    void PacketServerCancelRender::handle(IPacketHandler &handler)
    {
        handler.log("Server canceled current rendering!");
        auto &client = dynamic_cast<ClusterClient &>(handler);
        client.cancelRenderJobs();
    }

    PacketID PacketServerCancelRender::getId() const
    {
        return (PacketID::SERVER_CANCEL_RENDER);
    }
}
