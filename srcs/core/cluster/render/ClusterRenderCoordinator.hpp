//
// Created by jazema on 5/16/26.
//

#ifndef CLUSTERRENDERCOORDINATOR_HPP
#define CLUSTERRENDERCOORDINATOR_HPP

#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <deque>
#include <mutex>
#include <unordered_map>
#include <unordered_set>

namespace rc
{
    class ClusterRenderCoordinator
    {
        public:
            struct TileJob
            {
                uint32_t tile_id = 0;
                uint32_t sample = 0;
                int start_x = 0;
                int start_y = 0;
                int end_x = 0;
                int end_y = 0;
            };

            void beginSample(uint32_t sample, int width, int height, int tile_size);
            bool popJob(TileJob &job);
            bool popRemoteJob(TileJob &job);
            bool markComplete(const TileJob &job);
            void requeueTimedOut(std::chrono::milliseconds timeout);

            bool isActive() const;
            bool isSampleComplete() const;
            bool waitForSampleCompletion(std::chrono::milliseconds timeout);
            void cancel();
        private:
            struct InFlight
            {
                TileJob job;
                std::chrono::steady_clock::time_point assigned;
            };

            bool isSampleCompleteLocked() const;

            std::deque<TileJob>                    _pending;
            std::unordered_map<uint32_t, InFlight> _inFlight;
            std::unordered_set<uint32_t>           _completedTiles;
            std::unordered_set<uint32_t>           _timedOut;
            size_t                            _total         = 0;
            uint32_t                          _currentSample = 0;
            bool                              _active        = false;
            bool                              _cancelled     = false;

            mutable std::mutex _mutex;
            std::condition_variable _cv;
    };
}

#endif
