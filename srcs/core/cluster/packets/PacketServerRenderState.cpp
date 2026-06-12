//
// Created by jazema on 5/17/26.
//

#include "PacketServerRenderState.hpp"

#include "../../../common/cluster/ByteBuffer.hpp"
#include "../../../common/cluster/IClusterClient.hpp"

namespace rc
{
    void PacketServerRenderState::deserialize(std::vector<uint8_t> data)
    {
        ByteBuffer buffer;

        buffer.data = data;
        this->state = buffer.readUInt16();
    }

    std::vector<uint8_t> PacketServerRenderState::serialize() const
    {
        ByteBuffer buffer;

        buffer.write(this->state);
        return (buffer.data);
    }

    void PacketServerRenderState::handle(IPacketHandler &handler)
    {
        handler.log("Server sent its internal state !");
        dynamic_cast<IClusterClient &>(handler).updateServerRenderState(static_cast<ServerRenderState>(this->state));
    }

    PacketID PacketServerRenderState::getId() const
    {
        return (PacketID::SERVER_RENDER_STATE);
    }
}
