//
// Created by jazema on 5/16/26.
//

#include "PacketFactory.hpp"

#include <functional>
#include <map>

#include "PacketClientFetchSceneData.hpp"
#include "PacketClientJoinRequest.hpp"
#include "PacketClientTileData.hpp"
#include "PacketServerCancelRender.hpp"
#include "PacketServerClusterData.hpp"
#include "PacketServerJoinResponse.hpp"
#include "PacketServerRenderRequest.hpp"
#include "PacketServerRenderState.hpp"
#include "PacketServerSceneData.hpp"
#include "../../../common/cluster/PacketID.hpp"

namespace rc
{
    using PacketConstructor = std::function<std::unique_ptr<IPacket>()>;

    std::unique_ptr<IPacket> PacketFactory::createPacket(PacketID id, const std::vector<uint8_t> &payload)
    {
        static const std::map<PacketID, PacketConstructor> _packets = {
            {
                PacketID::CLIENT_JOIN_REQUEST, []
                {
                    return (std::make_unique<PacketClientJoinRequest>());
                }
            },
            {
                PacketID::SERVER_JOIN_RESPONSE, []
                {
                    return (std::make_unique<PacketServerJoinResponse>());
                }
            },
            {
                PacketID::CLIENT_FETCH_SCENE_DATA, []
                {
                    return (std::make_unique<PacketClientFetchSceneData>());
                }
            },
            {
                PacketID::SERVER_CLUSTER_DATA, []
                {
                    return (std::make_unique<PacketServerClusterData>());
                }
            },
            {
                PacketID::SERVER_SCENE_DATA, []
                {
                    return (std::make_unique<PacketServerSceneData>());
                }
            },
            {
                PacketID::SERVER_RENDER_REQUEST, []
                {
                    return (std::make_unique<PacketServerRenderRequest>());
                }
            },
            {
                PacketID::CLIENT_TILE_DATA, []
                {
                    return (std::make_unique<PacketClientTileData>());
                }
            },
            {
                PacketID::SERVER_CANCEL_RENDER, []
                {
                    return (std::make_unique<PacketServerCancelRender>());
                }
            },
            {
                PacketID::SERVER_RENDER_STATE, []
                {
                    return (std::make_unique<PacketServerRenderState>());
                }
            }
        };

        if (_packets.find(id) != _packets.end())
        {
            std::unique_ptr<IPacket> result = _packets.at(id)();
            result->deserialize(payload);
            return (result);
        }
        return (nullptr);
    }
}
