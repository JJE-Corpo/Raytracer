//
// Created by jazema on 5/16/26.
//

#include "PacketServerSceneData.hpp"

#include <iostream>

#include "../../../common/cluster/ByteBuffer.hpp"
#include "../../../common/cluster/IClusterClient.hpp"
#include "../../scene/SceneParser.hpp"

namespace rc
{
    void PacketServerSceneData::deserialize(std::vector<uint8_t> data)
    {
        ByteBuffer buffer;

        buffer.data = data;
        this->sceneData = buffer.readString();
    }

    std::vector<uint8_t> PacketServerSceneData::serialize() const
    {
        ByteBuffer buffer;

        buffer.writeString(this->sceneData);
        return (buffer.data);
    }

    void PacketServerSceneData::handle(IPacketHandler &handler)
    {
        SceneParser parser = SceneParser();
        libconfig::Config config = libconfig::Config();
        auto &asClient = dynamic_cast<IClusterClient &>(handler);

        handler.log("Server sent scene informations!");
        asClient.setClientState(IClusterClient::ClientState::RECEIVING_DATA);
        config.readString(this->sceneData);
        try
        {
            asClient.useScene(parser.parseScene(config));
            asClient.setClientState(IClusterClient::ClientState::IDLING);
        }
        catch (std::exception &e)
        {
            std::cerr << "Error receiving scene from server" << std::endl;
        }
    }

    PacketID PacketServerSceneData::getId() const
    {
        return (PacketID::SERVER_SCENE_DATA);
    }
}
