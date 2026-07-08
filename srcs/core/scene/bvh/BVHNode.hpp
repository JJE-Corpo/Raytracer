//
// Created by jazema on 5/1/26.
//

#ifndef BVHNODE_HPP
#define BVHNODE_HPP
#include "../../../common/scene/IPrimitive.hpp"

namespace rc
{
    struct BVHBuildItem
    {
        IPrimitive *primitive;
        AABB bounds;
        Vector3f centroid;
    };

    class BVHNode : public IPrimitive
    {
        private:
            IPrimitive *_left = nullptr;
            IPrimitive *_right = nullptr;
            AABB _bbox;
            bool _hidden = false;
            // True when _left/_right are child BVHNodes this node allocated (and
            // therefore owns); false for leaves, which point at primitives owned
            // elsewhere (the scene / the mesh's triangles) and must not be freed.
            bool _internal = false;
        public:
            BVHNode(std::vector<BVHBuildItem> &objects, int start, int end);
            ~BVHNode();

            bool intersect(const Ray &ray, float tMin, float tMax, Intersection &hit) const override;
            bool isFinite() const override;
            AABB bounding_box() const override;

            std::string getName() const override;
            void setName(const std::string &name) override { (void)name; }
            std::string getTypeName() const override;
            Vector3f getPosition() const override;
            Vector3f getRotation() const override;
            Vector3f getScale() const override;
            void setPosition(const Vector3f &position) override;
            void setRotation(const Vector3f &rotation) override;
            void setScale(const Vector3f &scale) override;

            std::map<std::string, std::pair<std::string, PropertyType>> getProperties() const override;
            void setPropertyFloat(const std::string &key, float value) override;
            const Material *getMaterial() const override;
            void setMaterial(const Material *material) override;
            bool isHidden() const override;
            void setHidden(bool hidden) override;

            ISceneObject *getParent() const override { return nullptr; }
            void setParent(ISceneObject *parent) override { (void)parent; }
            const std::vector<ISceneObject *> &getChildren() const override
            {
                static const std::vector<ISceneObject *> empty;
                return empty;
            }
            void addChild(ISceneObject *child) override { (void)child; }
            void insertChild(ISceneObject *child, std::size_t index) override { (void)child; (void)index; }
            void removeChild(ISceneObject *child) override { (void)child; }
            Vector3f getLocalPosition() const override { return {0.0f, 0.0f, 0.0f}; }
            Vector3f getLocalRotation() const override { return {0.0f, 0.0f, 0.0f}; }
            Vector3f getLocalScale() const override { return {1.0f, 1.0f, 1.0f}; }
            void setLocalPosition(const Vector3f &position) override { (void)position; }
            void setLocalRotation(const Vector3f &rotation) override { (void)rotation; }
            void setLocalScale(const Vector3f &scale) override { (void)scale; }
    };
}

#endif
