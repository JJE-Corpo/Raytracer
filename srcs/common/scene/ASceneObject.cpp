//
// Shared base for scene-graph nodes.
//

#include "ASceneObject.hpp"

#include <algorithm>

namespace rc
{
    void ASceneObject::addChild(ISceneObject *child)
    {
        if (!child)
            return;
        if (std::find(this->_children.begin(), this->_children.end(), child) == this->_children.end())
            this->_children.push_back(child);
        child->setParent(this);
    }

    void ASceneObject::insertChild(ISceneObject *child, std::size_t index)
    {
        if (!child)
            return;
        // Drop any existing slot first so a re-insert (reordering within the
        // same parent) interprets the index against the list without a stale
        // duplicate, then splice it back in at the requested position.
        auto existing = std::find(this->_children.begin(), this->_children.end(), child);
        if (existing != this->_children.end())
            this->_children.erase(existing);
        if (index > this->_children.size())
            index = this->_children.size();
        this->_children.insert(this->_children.begin() + static_cast<std::ptrdiff_t>(index), child);
        child->setParent(this);
    }

    void ASceneObject::removeChild(ISceneObject *child)
    {
        auto it = std::find(this->_children.begin(), this->_children.end(), child);
        if (it != this->_children.end())
            this->_children.erase(it);
        if (child && child->getParent() == this)
            child->setParent(nullptr);
    }

    // When parented, the argument is a local (parent-relative) transform. When a
    // root, local == world, so write straight through to the world transform
    // (dispatches to the leaf's own setter).
    void ASceneObject::setLocalPosition(const Vector3f &position)
    {
        if (this->_parent)
            this->_localPosition = position;
        else
            this->setPosition(position);
    }

    void ASceneObject::setLocalRotation(const Vector3f &rotation)
    {
        if (this->_parent)
            this->_localRotation = rotation;
        else
            this->setRotation(rotation);
    }

    void ASceneObject::setLocalScale(const Vector3f &scale)
    {
        if (this->_parent)
            this->_localScale = scale;
        else
            this->setScale(scale);
    }
}
