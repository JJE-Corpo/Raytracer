//
// Created by jazema on 5/13/26.
//

#ifndef CLUSTERCLIENT_HPP
#define CLUSTERCLIENT_HPP
#include <atomic>
#include <condition_variable>
#include <deque>
#include <functional>
#include <mutex>
#include <string>
#include <thread>

#include "../../../common/cluster/ByteBuffer.hpp"
#include "../../../common/cluster/ConnectionState.hpp"
#include "../../../common/cluster/IClusterClient.hpp"
#include "../../../common/scene/IScene.hpp"

namespace rc
{
    class ClusterClient : public IClusterClient
    {
        public:
            struct RenderJob
            {
                uint32_t tile_id = 0;
                uint32_t sample = 0;
                int start_x = 0;
                int start_y = 0;
                int end_x = 0;
                int end_y = 0;
            };

        private:
            std::string _address;
            std::uint16_t _port{};
            int _socketFd;
            ConnectionState _connectionState;

            IScene *_scene;

            ClientState _clientState = ClientState::IDLING;
            ServerRenderState _serverState = ServerRenderState::IDLING;

            ByteBuffer _writeBuffer;
            ByteBuffer _readBuffer;

            std::mutex _writeMutex;

            std::deque<RenderJob> _renderQueue;
            std::mutex _renderMutex;
            std::condition_variable _renderCv;
            std::thread _renderThread;
            std::atomic<bool> _renderRunning{false};

            std::thread _thread;
            std::atomic<bool> _running{false};
            mutable std::mutex _stateMutex;

            std::function<void(const std::string &message)> _infoToast;
            std::function<void(const std::string &message)> _errorToast;

            void run();
            void startThread();
            void renderLoop();
            void startRenderThread();
            void stopRenderThread();
        public:
            ClusterClient();
            ~ClusterClient() override;

            void join(const std::string &address, size_t port) override;
            void disconnect() override;

            void sendPacket(const IPacket &packet) override;

            bool handleRead();
            bool handleWrite();

            std::string getAddress() const override;
            uint16_t getPort() const override;

            ClientState getClientState() const override;
            void setClientState(ClientState clientState) override;
            std::string getStatus() const override;

            void useScene(IScene *scene) override;
            IScene *getScene() const override;

            void enqueueRenderJob(const RenderJob &job);
            void cancelRenderJobs();

            ConnectionState getConnectionState() const override;
            void setConnectionState(ConnectionState connectionState) override;

            void updateServerRenderState(ServerRenderState renderState) override;
            ServerRenderState getServerRenderState() const override;

            void setInfoLogger(std::function<void(const std::string &message)> infoToast) override;
            void setErrorLogger(std::function<void(const std::string &message)> infoToast) override;
            void log(const std::string &message) override;
    };
}

#endif
