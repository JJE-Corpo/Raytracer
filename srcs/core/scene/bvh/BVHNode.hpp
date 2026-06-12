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
            IPrimitive *_left;
            IPrimitive *_right;
            AABB _bbox;
            bool _hidden = false;
        public:
            BVHNode(std::vector<BVHBuildItem> &objects, int start, int end);

            bool intersect(const Ray &ray, float tMin, float tMax, Intersection &hit) const override;
            bool isFinite() const override;
            AABB bounding_box() const override;

            std::string getName() const override;
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
    };
}

#endif
