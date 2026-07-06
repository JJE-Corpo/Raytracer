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
            // Local worker: takes the next pending tile (including tiles that
            // were reclaimed from a timed-out client).
            bool popJob(TileJob &job);
            // Remote dispatch: takes the next pending tile that has NOT already
            // timed out once. A tile reclaimed after a timeout is left for the
            // local worker so a dead/silent client can never re-grab it forever.
            bool popRemoteJob(TileJob &job);
            // Returns true only if this call is the one that newly completed the
            // tile for the current sample, so the caller can decide whether to
            // apply the pixels (prevents double accumulation when a tile is
            // rendered twice after a timeout requeue).
            bool markComplete(const TileJob &job);
            // Moves tiles that were handed out but not completed within the
            // timeout back into the pending queue so another worker (a client or
            // the local render loop) can finish them. Guarantees the sample
            // eventually completes even if a client dies mid-render.
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
