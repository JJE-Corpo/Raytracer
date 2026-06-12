//
// Created by jazema on 5/14/26.
//

#include "PacketClientJoinRequest.hpp"

#include <filesystem>
#include <fstream>

#include "PacketServerJoinResponse.hpp"
#include "PacketServerSceneData.hpp"
#include "../../../common/cluster/ByteBuffer.hpp"
#include "../../../common/cluster/ConnectionState.hpp"
#include "../../../common/cluster/IClusterServer.hpp"
#include "../../../common/cluster/PacketID.hpp"
#include "../../scene/SceneRegister.hpp"
#include "../server/Connection.hpp"

namespace rc
{
    void PacketClientJoinRequest::deserialize(std::vector<uint8_t> data)
    {
        ByteBuffer buffer;

        buffer.data = data;
    }

    std::vector<uint8_t> PacketClientJoinRequest::serialize() const
    {
        ByteBuffer buffer;

        return (buffer.data);
    }

    void PacketClientJoinRequest::handle(IPacketHandler &handler)
    {
        handler.log("Asked to connect !");
        handler.setConnectionState(ConnectionState::CONNECTED);
        handler.sendPacket(PacketServerJoinResponse());
    }

    PacketID PacketClientJoinRequest::getId() const
    {
        return (PacketID::CLIENT_JOIN_REQUEST);
    }
}
