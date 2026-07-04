//
// Created by jazema on 5/15/26.
//

#ifndef CLUSTERMODULE_HPP
#define CLUSTERMODULE_HPP
#include "../../common/cluster/IClusterModule.hpp"

namespace rc
{
    class ClusterModule : public IClusterModule
    {
        private:
            ClusterMode _clusterMode = ClusterMode::NONE;
            IClusterServer *_clusterServer = nullptr;
            IClusterClient *_clusterClient = nullptr;
        public:
            ~ClusterModule();

            ClusterMode getClusterMode() const override;

            IClusterServer *getClusterServer() const override;
            IClusterClient *getClusterClient() const override;

            void startServer(IScene *scene, size_t port = 0) override;
            void joinCluster(const std::string &address, size_t port) override;
            void leaveCluster() override;
    };
}

#endif
