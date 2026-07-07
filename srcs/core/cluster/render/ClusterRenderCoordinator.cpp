//
// Created by jazema on 5/16/26.
//

#include "ClusterRenderCoordinator.hpp"

#include <algorithm>

namespace rc
{
    void ClusterRenderCoordinator::beginSample(uint32_t sample, int width, int height, int tile_size)
    {
        std::lock_guard lock(this->_mutex);

        this->_pending.clear();
        this->_inFlight.clear();
        this->_completedTiles.clear();
        this->_timedOut.clear();
        this->_currentSample = sample;
        this->_active = true;
        this->_cancelled = false;

        int tiles_x = (width + tile_size - 1) / tile_size;
        int tiles_y = (height + tile_size - 1) / tile_size;
        this->_total = static_cast<size_t>(tiles_x * tiles_y);

        uint32_t tile_id = 0;
        for (int ty = 0; ty < tiles_y; ++ty)
        {
            for (int tx = 0; tx < tiles_x; ++tx)
            {
                TileJob job;
                job.tile_id = tile_id++;
                job.sample = sample;
                job.start_x = tx * tile_size;
                job.start_y = ty * tile_size;
                job.end_x = std::min(job.start_x + tile_size, width);
                job.end_y = std::min(job.start_y + tile_size, height);
                this->_pending.push_front(job);
            }
        }

        this->_cv.notify_all();
    }

    bool ClusterRenderCoordinator::popJob(TileJob &job)
    {
        std::lock_guard lock(this->_mutex);
        if (this->_pending.empty())
            return (false);
        job = this->_pending.front();
        this->_pending.pop_front();
        this->_inFlight[job.tile_id] = InFlight{job, std::chrono::steady_clock::now()};
        return (true);
    }

    bool ClusterRenderCoordinator::popRemoteJob(TileJob &job)
    {
        std::lock_guard lock(this->_mutex);
        for (auto it = this->_pending.begin(); it != this->_pending.end(); ++it)
        {
            if (this->_timedOut.count(it->tile_id) != 0)
                continue;
            job = *it;
            this->_pending.erase(it);
            this->_inFlight[job.tile_id] = InFlight{job, std::chrono::steady_clock::now()};
            return (true);
        }
        return (false);
    }

    bool ClusterRenderCoordinator::markComplete(const TileJob &job)
    {
        std::lock_guard lock(this->_mutex);

        if (!this->_active || this->_cancelled)
            return (false);
        if (job.sample != this->_currentSample)
            return (false);
        this->_inFlight.erase(job.tile_id);
        if (this->_completedTiles.count(job.tile_id) != 0)
            return (false);
        this->_completedTiles.insert(job.tile_id);
        if (this->isSampleCompleteLocked())
            this->_cv.notify_all();
        return (true);
    }

    void ClusterRenderCoordinator::requeueTimedOut(std::chrono::milliseconds timeout)
    {
        std::lock_guard lock(this->_mutex);

        if (!this->_active || this->_cancelled)
            return ;

        const auto now = std::chrono::steady_clock::now();
        bool requeued = false;

        for (auto it = this->_inFlight.begin(); it != this->_inFlight.end();)
        {
            if (this->_completedTiles.count(it->first) != 0)
            {
                it = this->_inFlight.erase(it);
                continue;
            }
            if (now - it->second.assigned >= timeout)
            {
                this->_pending.push_front(it->second.job);
                this->_timedOut.insert(it->first);
                it = this->_inFlight.erase(it);
                requeued = true;
                continue;
            }
            ++it;
        }
        if (requeued)
            this->_cv.notify_all();
    }

    bool ClusterRenderCoordinator::isActive() const
    {
        std::lock_guard lock(this->_mutex);
        return (this->_active && !this->_cancelled);
    }

    bool ClusterRenderCoordinator::isSampleCompleteLocked() const
    {
        return (this->_active && !this->_cancelled && this->_completedTiles.size() >= this->_total && this->_pending.empty());
    }

    bool ClusterRenderCoordinator::isSampleComplete() const
    {
        std::lock_guard lock(this->_mutex);
        return this->isSampleCompleteLocked();
    }

    bool ClusterRenderCoordinator::waitForSampleCompletion(std::chrono::milliseconds timeout)
    {
        std::unique_lock lock(this->_mutex);
        if (this->isSampleCompleteLocked())
            return (true);
        this->_cv.wait_for(lock, timeout, [&]
        {
            return (this->isSampleCompleteLocked() || this->_cancelled);
        });
        return (this->isSampleCompleteLocked());
    }

    void ClusterRenderCoordinator::cancel()
    {
        std::lock_guard lock(this->_mutex);
        this->_pending.clear();
        this->_inFlight.clear();
        this->_completedTiles.clear();
        this->_timedOut.clear();
        this->_active = false;
        this->_cancelled = true;
        this->_cv.notify_all();
    }
}
