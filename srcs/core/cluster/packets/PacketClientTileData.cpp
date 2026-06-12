//
// Created by jazema on 5/16/26.
//

#include "PacketClientTileData.hpp"

#include "../../../common/cluster/ByteBuffer.hpp"
#include "../../../common/cluster/PacketID.hpp"
#include "../server/Connection.hpp"
#include "../server/ClusterServer.hpp"

namespace rc
{
    void PacketClientTileData::deserialize(std::vector<uint8_t> data)
    {
        ByteBuffer buffer;

        buffer.data = data;
        this->tile_id = buffer.readUInt32();
        this->sample = buffer.readUInt32();
        this->start_x = buffer.readUInt32();
        this->start_y = buffer.readUInt32();
        this->end_x = buffer.readUInt32();
        this->end_y = buffer.readUInt32();

        const uint32_t tile_w = this->end_x - this->start_x;
        const uint32_t tile_h = this->end_y - this->start_y;
        const size_t count = (tile_w * tile_h);

        this->pixels.clear();
        this->pixels.reserve(count);
        for (size_t i = 0; i < count; ++i)
        {
            ColorF c{};
            c.r = buffer.readFloat();
            c.g = buffer.readFloat();
            c.b = buffer.readFloat();
            this->pixels.push_back(c);
        }
    }

    std::vector<uint8_t> PacketClientTileData::serialize() const
    {
        ByteBuffer buffer;

        buffer.write(this->tile_id);
        buffer.write(this->sample);
        buffer.write(this->start_x);
        buffer.write(this->start_y);
        buffer.write(this->end_x);
        buffer.write(this->end_y);

        for (const auto &c : this->pixels)
        {
            buffer.write(c.r);
            buffer.write(c.g);
            buffer.write(c.b);
        }

        return (buffer.data);
    }

    void PacketClientTileData::handle(IPacketHandler &handler)
    {
        auto &connection = dynamic_cast<Connection &>(handler);
        auto *server = dynamic_cast<ClusterServer *>(connection.getServer());

        handler.log("Received tile data from client (tile " + std::to_string(this->tile_id) + ")");
        if (!server)
            return;
        server->handleClientTileData(connection.getFd(), *this);
    }

    PacketID PacketClientTileData::getId() const
    {
        return (PacketID::CLIENT_TILE_DATA);
    }
}
