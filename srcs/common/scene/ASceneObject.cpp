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
