//
// Created by jazema on 5/10/26.
//

#ifndef ISCENEOBJECT_HPP
#define ISCENEOBJECT_HPP
#include <string>
#include <vector>

#include "../Vector.hpp"

namespace rc
{
    enum class ObjectType
    {
        PRIMITIVE,
        LIGHT,
        GROUP,
    };

    class ISceneObject
    {
        public:
            virtual ~ISceneObject() = default;

            virtual ObjectType getObjectType() const = 0;

            // WORLD transform: what rendering (intersect/BVH) reads. For a leaf
            // these are its own geometry fields; the flatten pass writes them
            // from the composed ancestor chain before each BVH build.
            virtual Vector3f getPosition() const = 0;
            virtual Vector3f getRotation() const = 0;
            virtual Vector3f getScale() const = 0;
            virtual bool isHidden() const = 0;

            virtual std::string getName() const = 0;
            virtual std::string getTypeName() const = 0;

            virtual void setPosition(const Vector3f &position) = 0;
            virtual void setRotation(const Vector3f &rotation) = 0;
            virtual void setScale(const Vector3f &scale) = 0;
            virtual void setHidden(bool hidden) = 0;
            virtual void setName(const std::string &name) = 0;

            // Scene-graph links. Parents are always groups (or null for roots).
            virtual ISceneObject *getParent() const = 0;
            virtual void setParent(ISceneObject *parent) = 0;
            virtual const std::vector<ISceneObject *> &getChildren() const = 0;
            virtual void addChild(ISceneObject *child) = 0;
            // Insert child at a specific slot among the existing children (index
            // clamped to [0, size]). Used to reorder siblings; addChild() is the
            // append shorthand.
            virtual void insertChild(ISceneObject *child, std::size_t index) = 0;
            virtual void removeChild(ISceneObject *child) = 0;

            // LOCAL transform: parent-relative, what the editor edits and what is
            // serialized. For a root (no parent) local == world.
            virtual Vector3f getLocalPosition() const = 0;
            virtual Vector3f getLocalRotation() const = 0;
            virtual Vector3f getLocalScale() const = 0;
            virtual void setLocalPosition(const Vector3f &position) = 0;
            virtual void setLocalRotation(const Vector3f &rotation) = 0;
            virtual void setLocalScale(const Vector3f &scale) = 0;
    };
}

#endif
