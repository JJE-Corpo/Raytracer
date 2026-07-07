#ifndef IEDITABLEPRIMITIVE_HPP
#define IEDITABLEPRIMITIVE_HPP

#include <cstddef>
#include <map>

#include "../Vector.hpp"

namespace rc
{
    class IEditablePrimitive
    {
        public:
            virtual ~IEditablePrimitive() = default;

            // Number of editable vertices (welded/shared vertices count once).
            virtual std::size_t getVertexCount() const = 0;

            virtual Vector3f getVertex(std::size_t index) const = 0;

            virtual void setVertex(std::size_t index, const Vector3f &worldPos) = 0;

            virtual void onGeometryChanged() = 0;

            virtual std::map<std::size_t, Vector3f> getVertexOverrides() const { return {}; }
    };
}

#endif
