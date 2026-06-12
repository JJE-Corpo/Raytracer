//
// Created by jazema on 5/14/26.
//

#ifndef PACKETID_HPP
#define PACKETID_HPP
#include <cstdint>

namespace rc
{
    enum class PacketID : uint8_t
    {
        CLIENT_JOIN_REQUEST,
        SERVER_JOIN_RESPONSE,

        SERVER_RENDER_REQUEST,
        SERVER_RENDER_STATE,
        SERVER_CANCEL_RENDER,

        CLIENT_TILE_DATA,
        SERVER_TILE_DATA,

        CLIENT_FETCH_CLUSTER_DATA,
        CLIENT_FETCH_SCENE_DATA,

        SERVER_SCENE_DATA,
        SERVER_CLUSTER_DATA,
    };
}

#endif
