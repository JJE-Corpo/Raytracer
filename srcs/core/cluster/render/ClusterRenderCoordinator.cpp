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
        this->_completedTiles.clear();
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
        return (true);
    }

    void ClusterRenderCoordinator::markComplete(const TileJob &job)
    {
        std::lock_guard lock(this->_mutex);

        if (job.sample != this->_currentSample)
            return ;
        if (std::find(this->_completedTiles.begin(), this->_completedTiles.end(), job.tile_id) != this->_completedTiles.end())
            return ;
        this->_completedTiles.insert(job.tile_id);
        if (this->isSampleCompleteLocked())
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
        this->_completedTiles.clear();
        this->_active = false;
        this->_cancelled = true;
        this->_cv.notify_all();
    }
}
