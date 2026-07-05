//
// Optional capability interface for primitives whose vertices can be moved
// interactively in the viewport (vertex edit mode).
//
// It is deliberately kept OUT of IPrimitive: IPrimitive is the contract shared
// with every plugin, so bolting vertex editing onto it would force all
// primitives (and any third-party plugin) to implement it. Instead a primitive
// that supports editing inherits IEditablePrimitive IN ADDITION to IPrimitive,
// and the UI discovers the capability at runtime with
// `dynamic_cast<IEditablePrimitive *>(primitive)` -- the same runtime-capability
// pattern the renderer already uses for ISelectionAwareRenderer.
//
// All positions are expressed in WORLD space: the UI projects them straight to
// screen for the handles and feeds back the world position picked under the
// cursor. A primitive that bakes a local transform (e.g. Mesh) converts to/from
// its own space internally.
//

#ifndef IEDITABLEPRIMITIVE_HPP
#define IEDITABLEPRIMITIVE_HPP

#include <cstddef>

#include "../Vector.hpp"

namespace rc
{
    class IEditablePrimitive
    {
        public:
            virtual ~IEditablePrimitive() = default;

            // Number of editable vertices (welded/shared vertices count once).
            virtual std::size_t getVertexCount() const = 0;

            // World-space position of vertex `index` (index < getVertexCount()).
            virtual Vector3f getVertex(std::size_t index) const = 0;

            // Move vertex `index` to `worldPos`. Every face incident to a shared
            // vertex must follow. Implementations should keep this cheap enough to
            // call on every drag frame and defer any heavy rebuild (acceleration
            // structures) to onGeometryChanged().
            virtual void setVertex(std::size_t index, const Vector3f &worldPos) = 0;

            // Signals the end of an interactive edit (mouse release). The
            // primitive may now do the work it deferred during the drag, e.g.
            // rebuild a local BVH. Safe to call at any time / repeatedly.
            virtual void onGeometryChanged() = 0;
    };
}

#endif
