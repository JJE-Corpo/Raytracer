//
// Created by jazema on 5/15/26.
//

#ifndef ICLUSTERSERVER_HPP
#define ICLUSTERSERVER_HPP

namespace rc
{
    class IScene;

    class IClusterServer
    {
        public:
            virtual ~IClusterServer() = default;

            virtual void start() = 0;
            virtual void stop() = 0;

            virtual IScene *getScene() = 0;

            virtual uint16_t getPort() const = 0;
    };
}

#endif
