//
// Created by jazema on 4/21/26.
//

#ifndef SCENE_HPP
#define SCENE_HPP
#include <memory>
#include <mutex>
#include <vector>

#include "../../common/scene/ILight.hpp"
#include "../../common/scene/IPrimitive.hpp"
#include "../../common/scene/IScene.hpp"
#include "Group.hpp"
#include "bvh/BVHNode.hpp"

namespace rc
{
    class Scene : public IScene
    {
        private:
            ICamera *_camera;
            std::vector<IPrimitive *> _primitives;
            std::vector<IPrimitive *> _infinitePrimitives;
            std::map<std::string, Material> _materials;
            std::vector<ILight *> _lights;
            // Top-level nodes (non-owning: they alias objects owned by
            // _primitives/_lights/_groups). Groups are owned here for cleanup.
            std::vector<ISceneObject *> _roots;
            std::vector<Group *> _groups;
            float _ambientCoefficient;
            float _diffuseCoefficient;

            std::unique_ptr<BVHNode> _bvhRoot;
            std::mutex _mutex;

            // Root-list helpers (callers already hold, or don't need, _mutex).
            void addRoot(ISceneObject *object);
            // Insert a top-level node at a specific slot; a negative or
            // out-of-range index appends (matching addRoot()).
            void insertRoot(ISceneObject *object, int index);
            void removeRoot(ISceneObject *object);
            // True while `object` is still held by one of the owning vectors
            // (_primitives/_lights/_groups). Compares by up-casting each owning
            // pointer to the shared base, never dereferencing `object`, so it is
            // safe to ask about a pointer that may already have been freed.
            bool isOwned(const ISceneObject *object) const;
            // Erase `object` and its whole subtree from the owning vectors and
            // delete them (children first). Does not touch the parent's child
            // list, so the caller must detach `object` beforehand. Not locked.
            void destroyObjectSubtree(ISceneObject *object);
            // Compose world transforms down the graph, writing world into each
            // node. Not locked: called from buildBvh() which already holds _mutex.
            void flattenGraph();
            void flattenNode(ISceneObject *object, const Vector3f &parentPos,
                const Vector3f &parentRot, const Vector3f &parentScale);
        public:
            Scene(ICamera *camera, float ambientCoefficient = 0.4f, float diffuseCoefficient = 0.6f);
            ~Scene();

            void addPrimitive(IPrimitive *primitive) override;
            void addLight(ILight *light) override;
            void addDefaultPrimitive(std::string type) override;
            void addDefaultLight(std::string type) override;
            ISceneObject *addGroup() override;
            void reparent(ISceneObject *child, ISceneObject *newParent, int index = -1) override;
            void removeObject(ISceneObject *object) override;
            IPrimitive *convertToMesh(IPrimitive *primitive) override;
            // Take ownership of an already-built group (used by the parser/builder
            // when reconstructing a saved hierarchy).
            void adoptGroup(Group *group);
            Material *getMaterial(const std::string &name) override;
            Material *createMaterial(const std::string &name) override;
            Material *createMaterial() override;
            void clear() override;

            float getAmbientCoefficient() const override;
            float getDiffuseCoefficient() const override;
            void setAmbientCoefficient(float ambientCoefficient) override;
            void setDiffuseCoefficient(float diffuseCoefficient) override;

            ICamera &getCamera() const override;
            const std::vector<IPrimitive *> &getPrimitives() const override;
            const std::vector<ILight *> &getLights() const override;
            const std::map<std::string, Material> &getMaterials() const override;
            const std::vector<ISceneObject *> &getRoots() const override;

            void buildBvh() override;
            bool intersect(const Ray& ray, float tMin, float tMax, Intersection& hit) const override;
    };
}

#endif
