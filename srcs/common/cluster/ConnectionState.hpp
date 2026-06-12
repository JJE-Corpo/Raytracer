//
// Created by jazema on 5/16/26.
//

#ifndef CONNECTIONSTATE_HPP
#define CONNECTIONSTATE_HPP

namespace rc
{
    enum class ConnectionState
    {
        PENDING,
        CONNECTED,
        DISCONNECTED,
        REFUSED,
    };
}

#endif
