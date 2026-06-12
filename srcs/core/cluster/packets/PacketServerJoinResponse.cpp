//
// Created by jazema on 5/16/26.
//

#include "PacketServerJoinResponse.hpp"

#include "PacketClientFetchSceneData.hpp"
#include "../../../common/cluster/ByteBuffer.hpp"
#include "../../../common/cluster/ConnectionState.hpp"
#include "../../../common/cluster/IClusterClient.hpp"

namespace rc
{
    std::vector<uint8_t> PacketServerJoinResponse::serialize() const
    {
        ByteBuffer buffer;

        return (buffer.data);
    }

    void PacketServerJoinResponse::deserialize(std::vector<uint8_t> data)
    {
        ByteBuffer buffer;

        buffer.data = data;
    }

    void PacketServerJoinResponse::handle(IPacketHandler &handler)
    {
        handler.log("Connection accepted by server!");
        handler.setConnectionState(ConnectionState::CONNECTED);
        dynamic_cast<IClusterClient &>(handler).setClientState(IClusterClient::ClientState::FETCHING_DATA);
        handler.sendPacket(PacketClientFetchSceneData());
    }

    PacketID PacketServerJoinResponse::getId() const
    {
        return (PacketID::SERVER_JOIN_RESPONSE);
    }
}
