//
// Created by jazema on 5/16/26.
//

#include "PacketServerRenderRequest.hpp"

#include "../../../common/cluster/ByteBuffer.hpp"
#include "../../../common/cluster/PacketID.hpp"
#include "../client/ClusterClient.hpp"

namespace rc
{
    void PacketServerRenderRequest::deserialize(std::vector<uint8_t> data)
    {
        ByteBuffer buffer;

        buffer.data = std::move(data);
        this->tile_id = buffer.readUInt32();
        this->sample = buffer.readUInt32();
        this->start_x = buffer.readUInt32();
        this->start_y = buffer.readUInt32();
        this->end_x = buffer.readUInt32();
        this->end_y = buffer.readUInt32();
    }

    std::vector<uint8_t> PacketServerRenderRequest::serialize() const
    {
        ByteBuffer buffer;

        buffer.write(this->tile_id);
        buffer.write(this->sample);
        buffer.write(this->start_x);
        buffer.write(this->start_y);
        buffer.write(this->end_x);
        buffer.write(this->end_y);
        return (buffer.data);
    }

    void PacketServerRenderRequest::handle(IPacketHandler &handler)
    {
        auto &client = dynamic_cast<ClusterClient &>(handler);
        ClusterClient::RenderJob job;

        handler.log("Server requesting for tile render! (tile " + std::to_string(this->tile_id) + ")");
        job.tile_id = this->tile_id;
        job.sample = this->sample;
        job.start_x = static_cast<int>(this->start_x);
        job.start_y = static_cast<int>(this->start_y);
        job.end_x = static_cast<int>(this->end_x);
        job.end_y = static_cast<int>(this->end_y);
        client.enqueueRenderJob(job);
    }

    PacketID PacketServerRenderRequest::getId() const
    {
        return (PacketID::SERVER_RENDER_REQUEST);
    }
}
