//
// Created by jazema on 5/15/26.
//

#include "ClusterModule.hpp"

#include <stdexcept>

#include "client/ClusterClient.hpp"
#include "server/ClusterServer.hpp"

namespace rc
{
    ClusterModule::~ClusterModule()
    {
        if (this->_clusterServer)
        {
            this->_clusterServer->stop();
            delete (this->_clusterServer);
            this->_clusterServer = nullptr;
        }
        if (this->_clusterClient)
        {
            delete (this->_clusterClient);
            this->_clusterClient = nullptr;
        }
    }

    ClusterMode ClusterModule::getClusterMode() const
    {
        return (this->_clusterMode);
    }

    IClusterServer *ClusterModule::getClusterServer() const
    {
        return (this->_clusterServer);
    }

    IClusterClient *ClusterModule::getClusterClient() const
    {
        return (this->_clusterClient);
    }

    void ClusterModule::startServer(IScene *scene, size_t port)
    {
        if (this->_clusterMode != ClusterMode::NONE)
            throw std::runtime_error("Already hosting / connected to a cluster !");
        this->_clusterServer = new ClusterServer(scene, static_cast<uint16_t>(port));
        try
        {
            this->_clusterServer->start();
        }
        catch (std::exception &e)
        {
            delete (this->_clusterServer);
            this->_clusterServer = nullptr;
            throw (std::runtime_error(e.what()));
        }
        this->_clusterMode = ClusterMode::SERVER;
    }

    void ClusterModule::joinCluster(const std::string &address, const size_t port)
    {
        if (this->_clusterMode != ClusterMode::NONE)
            throw std::runtime_error("Already hosting / connected to a cluster !");
        this->_clusterClient = new ClusterClient();
        try
        {
            this->_clusterClient->join(address, port);
        }
        catch (std::exception &e)
        {
            delete (this->_clusterClient);
            this->_clusterClient = nullptr;
            throw (std::runtime_error(e.what()));
        }
        this->_clusterMode = ClusterMode::CLIENT;
    }

    void ClusterModule::leaveCluster()
    {
        if (this->_clusterMode != ClusterMode::CLIENT)
            throw std::runtime_error("Not connected to a cluster");
        if (this->_clusterClient)
        {
            this->_clusterClient->disconnect();
            delete (this->_clusterClient);
            this->_clusterClient = nullptr;
        }
        this->_clusterMode = ClusterMode::NONE;
    }
}
