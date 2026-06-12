//
// Created by jazema on 5/1/26.
//

#include "BVHNode.hpp"

#include <algorithm>
#include <cfloat>

#include "../../../common/Intersection.hpp"

namespace rc
{
    namespace
    {
        int longest_axis(const Vector3f &extent)
        {
            if (extent.x >= extent.y && extent.x >= extent.z)
                return (0);
            if (extent.y >= extent.z)
                return (1);
            return (2);
        }
    }

    BVHNode::BVHNode(std::vector<BVHBuildItem> &objects, int start, int end)
    {
        int objectSpan = end - start;
        int axis = 0;

        if (objectSpan > 1)
        {
            Vector3f centroid_min(FLT_MAX, FLT_MAX, FLT_MAX);
            Vector3f centroid_max(-FLT_MAX, -FLT_MAX, -FLT_MAX);

            for (int i = start; i < end; ++i)
            {
                const Vector3f &centroid = objects[i].centroid;
                centroid_min.x = std::min(centroid_min.x, centroid.x);
                centroid_min.y = std::min(centroid_min.y, centroid.y);
                centroid_min.z = std::min(centroid_min.z, centroid.z);
                centroid_max.x = std::max(centroid_max.x, centroid.x);
                centroid_max.y = std::max(centroid_max.y, centroid.y);
                centroid_max.z = std::max(centroid_max.z, centroid.z);
            }

            const Vector3f extent = centroid_max - centroid_min;
            axis = longest_axis(extent);
        }

        auto comparator = [axis](const BVHBuildItem &a, const BVHBuildItem &b) {
            return (a.centroid[axis] < b.centroid[axis]);
        };

        if (objectSpan <= 0)
            return;
            //throw std::runtime_error("No objects to build BVHNode");
        if (objectSpan == 1)
        {
            this->_left = this->_right = objects[start].primitive;
            this->_bbox = objects[start].bounds;
        }
        else if (objectSpan == 2)
        {
            if (comparator(objects[start], objects[start + 1]))
            {
                this->_left = objects[start].primitive;
                this->_right = objects[start + 1].primitive;
            }
            else
            {
                this->_left = objects[start + 1].primitive;
                this->_right = objects[start].primitive;
            }

            this->_bbox = BoundingBoxUtils::surrounding_box(objects[start].bounds, objects[start + 1].bounds);
        }
        else
        {
            int mid = start + objectSpan / 2;
            std::nth_element(objects.begin() + start, objects.begin() + mid, objects.begin() + end, comparator);
            this->_left = new BVHNode(objects, start, mid);
            this->_right = new BVHNode(objects, mid, end);
            this->_bbox = BoundingBoxUtils::surrounding_box(this->_left->bounding_box(), this->_right->bounding_box());
            return;
        }
    }

    bool BVHNode::intersect(const Ray &ray, float tMin, float tMax, Intersection &hit) const
    {
        if (!this->_bbox.hit(ray, tMin, tMax))
            return false;

        bool hit_left = this->_left->intersect(ray, tMin, tMax, hit);
        bool hit_right = this->_right->intersect(ray, tMin, hit_left ? hit.t : tMax, hit);

        return (hit_left || hit_right);
    }

    bool BVHNode::isFinite() const
    {
        return (true);
    }

    AABB BVHNode::bounding_box() const
    {
        return (this->_bbox);
    }

    std::string BVHNode::getName() const
    {
        return ("BVH Node");
    }

    std::string BVHNode::getTypeName() const
    {
        return "bvh_node";
    }

    Vector3f BVHNode::getPosition() const
    {
        return (Vector3f{0.0f, 0.0f, 0.0f});
    }

    Vector3f BVHNode::getRotation() const
    {
        return (Vector3f{0.0f, 0.0f, 0.0f});
    }

    Vector3f BVHNode::getScale() const
    {
        return (Vector3f{1.0f, 1.0f, 1.0f});
    }

    void BVHNode::setPosition(const Vector3f &position)
    {
        (void)position;
    }

    void BVHNode::setRotation(const Vector3f &rotation)
    {
        (void)rotation;
    }

    void BVHNode::setScale(const Vector3f &scale)
    {
        (void)scale;
    }

    std::map<std::string, std::pair<std::string, PropertyType>> BVHNode::getProperties() const
    {
        return {};
    }

    void BVHNode::setPropertyFloat(const std::string &, float)
    {
    }

    const Material *BVHNode::getMaterial() const
    {
        static Material empty_material;
        return &empty_material;
    }

    void BVHNode::setMaterial(const Material *material)
    {
        (void)material;
    }

    bool BVHNode::isHidden() const
    {
        return this->_hidden;
    }

    void BVHNode::setHidden(bool hidden)
    {
        this->_hidden = hidden;
    }
}
