//
// Created by jazema on 4/22/26.
//

#ifndef ISCENE_HPP
#define ISCENE_HPP

#include "ICamera.hpp"
#include "ILight.hpp"
#include "IPrimitive.hpp"
#include <vector>

namespace rc
{
    class IScene
    {
        public:
            virtual ~IScene() = default;

            virtual void addPrimitive(IPrimitive *primitive) = 0;
            virtual void addLight(ILight *light) = 0;
            virtual void addDefaultPrimitive(std::string type) = 0;
            virtual void addDefaultLight(std::string type) = 0;

            // Scene-graph operations. addGroup() creates an empty top-level group
            // and returns it. reparent() moves child under newParent (nullptr =
            // top level), preserving the child's world transform. `index` is the
            // slot among the destination siblings (roots when newParent is null),
            // counted without `child`; a negative index (the default) appends.
            // A same-parent move is an in-place reorder and leaves the transform
            // untouched.
            virtual ISceneObject *addGroup() = 0;
            virtual void reparent(ISceneObject *child, ISceneObject *newParent, int index = -1) = 0;
            virtual Material *getMaterial(const std::string &name) = 0;
            virtual Material *createMaterial(const std::string &name) = 0;
            virtual Material *createMaterial() = 0;
            virtual void clear() = 0;

            virtual float getAmbientCoefficient() const = 0;
            virtual float getDiffuseCoefficient() const = 0;
            virtual void setAmbientCoefficient(float ambientCoefficient) = 0;
            virtual void setDiffuseCoefficient(float diffuseCoefficient) = 0;

            virtual ICamera &getCamera() const = 0;
            virtual const std::vector<IPrimitive *> &getPrimitives() const = 0;
            virtual const std::vector<ILight *> &getLights() const = 0;
            virtual const std::map<std::string, Material> &getMaterials() const = 0;

            // Top-level scene-graph nodes (un-parented objects and groups), in
            // insertion order. The hierarchy panel and serializer walk this.
            virtual const std::vector<ISceneObject *> &getRoots() const = 0;

            virtual void buildBvh() = 0;

            virtual bool intersect(const Ray &ray, float tMin, float tMax, Intersection &hit) const = 0;
    };
}

#endif
