//
// Created by jazema on 5/7/26.
//

#ifndef ISELECTIONAWARE_RENDERER_HPP
#define ISELECTIONAWARE_RENDERER_HPP

#include <vector>

namespace rc
{
    class IPrimitive;
    class ILight;

    class ISelectionAwareRenderer
    {
        public:
            virtual ~ISelectionAwareRenderer() = default;
            virtual void setSelection(const std::vector<const ISceneObject *> &selection) = 0;
    };
}

#endif
