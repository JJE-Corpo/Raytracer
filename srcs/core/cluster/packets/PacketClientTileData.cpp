//
// Created by jazema on 5/16/26.
//

#include "PacketClientTileData.hpp"

#include <stdexcept>

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

        // Reject bounds that would underflow (end < start) or claim more pixels
        // than the payload actually carries, so we never reserve a bogus size or
        // read past the buffer. Each pixel is 3 floats = 12 bytes.
        if (this->end_x < this->start_x || this->end_y < this->start_y)
            throw std::runtime_error("Invalid tile bounds in tile data packet");

        const uint64_t tile_w = static_cast<uint64_t>(this->end_x) - this->start_x;
        const uint64_t tile_h = static_cast<uint64_t>(this->end_y) - this->start_y;
        const uint64_t count = tile_w * tile_h;
        const size_t remaining = buffer.data.size() - static_cast<size_t>(buffer.pos);

        if (count > remaining / (3 * sizeof(float)))
            throw std::runtime_error("Truncated pixel data in tile data packet");

        this->pixels.clear();
        this->pixels.reserve(static_cast<size_t>(count));
        for (uint64_t i = 0; i < count; ++i)
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
