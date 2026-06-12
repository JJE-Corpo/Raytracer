//
// Created by jazema on 5/16/26.
//

#ifndef ICLUSTERTILESINK_HPP
#define ICLUSTERTILESINK_HPP

#include <vector>

#include "../../../common/Color.hpp"
#include "ClusterRenderCoordinator.hpp"

namespace rc
{
    class IClusterTileSink
    {
        public:
            virtual ~IClusterTileSink() = default;
            virtual void applyTileSample(const ClusterRenderCoordinator::TileJob &job,
                const std::vector<ColorF> &pixels) = 0;
    };
}

#endif
