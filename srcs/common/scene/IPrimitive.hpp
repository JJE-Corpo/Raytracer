//
// Created by jazema on 4/21/26.
//

#ifndef IDISPLAYSHAPE_HPP
#define IDISPLAYSHAPE_HPP
#include "../IPlugin.hpp"

#include <string>
#include <map>

#include "../AABB.hpp"
#include "ISceneObject.hpp"
#include "../Material.hpp"

namespace rc
{
    struct Intersection;
    class Ray;

    enum class PropertyType
    {
        COLOR,
        VECTOR3F,
        VECTOR3I,
        VECTOR2F,
        VECTOR2I,
        FLOAT,
        INT,
        STRING
    };

    class IPrimitive : public ISceneObject
    {
        public:
            virtual ~IPrimitive() = default;

            ObjectType getObjectType() const override
            {
                return ObjectType::PRIMITIVE;
            };

            /**
             * Tests whether a ray intersects this primitive within a valid range.
             *
             * @param ray   The ray to test against the primitive.
             * @param tMin  Minimum valid distance along the ray (used to avoid
             *              self-intersections and numerical precision issues).
             * @param tMax  Maximum valid distance along the ray (used to limit
             *              intersection search, e.g. closest hit or shadow rays).
             * @param hit   Output structure that stores intersection data (such as
             *              hit point, normal, and distance). Only valid if the
             *              function returns true.
             *
             * @return true if the ray intersects the primitive within [tMin, tMax],
             *         false otherwise.
             *
             * This function is typically used to:
             * - Find the closest visible surface hit by a ray
             * - Filter out invalid or behind-camera intersections
             * - Support shadow ray checks and recursive ray tracing
             */
            virtual bool intersect(const Ray& ray, float tMin, float tMax, Intersection &hit) const = 0;

            virtual AABB bounding_box() const = 0;
            virtual bool isFinite() const = 0;

            virtual std::map<std::string, std::pair<std::string, PropertyType>> getProperties() const = 0;
            virtual void setPropertyFloat(const std::string &key, float value) = 0;

            virtual const Material *getMaterial() const = 0;
            virtual void setMaterial(const Material *material) = 0;
    };
}

#endif
