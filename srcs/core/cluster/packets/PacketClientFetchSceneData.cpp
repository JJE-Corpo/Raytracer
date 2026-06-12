//
// Created by jazema on 5/16/26.
//

#include "PacketClientFetchSceneData.hpp"

#include "PacketServerSceneData.hpp"
#include "../../../common/cluster/ByteBuffer.hpp"
#include "../../../common/cluster/IClusterServer.hpp"
#include "../../scene/SceneRegister.hpp"
#include "../server/Connection.hpp"

namespace rc
{
    void PacketClientFetchSceneData::deserialize(std::vector<uint8_t> data)
    {
        ByteBuffer buffer;

        buffer.data = data;
    }

    std::vector<uint8_t> PacketClientFetchSceneData::serialize() const
    {
        ByteBuffer buffer;

        return (buffer.data);
    }

    void PacketClientFetchSceneData::handle(IPacketHandler &handler)
    {
        PacketServerSceneData data;

        handler.log("Fetching scene data..");
        data.sceneData = SceneRegister().toString(dynamic_cast<Connection &>(handler).getServer()->getScene());
        handler.sendPacket(data);
    }

    PacketID PacketClientFetchSceneData::getId() const
    {
        return (PacketID::CLIENT_FETCH_SCENE_DATA);
    }
}
