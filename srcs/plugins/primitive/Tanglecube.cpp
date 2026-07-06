#include "Tanglecube.hpp"

#include <iostream>

#include "../../common/Intersection.hpp"
#include "../../common/AABB.hpp"
#include "../../common/IPlugin.hpp"
#include "../../common/UvMapping.hpp"
#include "../../core/scene/builder/SceneObjectBuilder.hpp"


rc::Tanglecube::Tanglecube(std::string name, Vector3f center, Vector3f rotation, float threshold, float size, const Material *material) : _center(center), _rotation(rotation), _threshold(threshold), _size(size), _invSize(1.0f / size), _material(material)
{
    if (!name.empty())
        this->_name = name;

    // std::cout << "Tanglecube constructed" << std::endl;
}

std::string rc::Tanglecube::getName() const
{
    return (this->_name);
}

std::string rc::Tanglecube::getTypeName() const
{
    return PRIMITIVE_TANGLECUBE;
}

rc::Vector3f rc::Tanglecube::getPosition() const
{
    return (this->_center);
}

rc::Vector3f rc::Tanglecube::getRotation() const
{
    return (this->_rotation);
}

rc::Vector3f rc::Tanglecube::getScale() const
{
    return (this->_scale);
}

void rc::Tanglecube::setPosition(const Vector3f &position)
{
    this->_center = position;
}

void rc::Tanglecube::setRotation(const Vector3f &rotation)
{
    this->_rotation = rotation;
}

void rc::Tanglecube::setScale(const Vector3f &scale)
{
    this->_scale = scale;
}

bool rc::Tanglecube::intersect(const Ray &ray, float tMin, float tMax, Intersection &hit) const
{
    const Vector3f localOrigin = (ray.origin - _center) * _invSize;
    const Vector3f localDir = ray.direction * _invSize;

    auto f = [&](const Vector3f &p) -> float
    {
        const float x2 = p.x * p.x;
        const float y2 = p.y * p.y;
        const float z2 = p.z * p.z;
        return x2 * x2 - 5.0f * x2 + y2 * y2 - 5.0f * y2 + z2 * z2 - 5.0f * z2 + _threshold;
    };

    auto gradient = [](const Vector3f &p) -> Vector3f
    {
        return Vector3f(
            4.0f * p.x * p.x * p.x - 10.0f * p.x,
            4.0f * p.y * p.y * p.y - 10.0f * p.y,
            4.0f * p.z * p.z * p.z - 10.0f * p.z
        );
    };

    const float a = dot(localDir, localDir);
    const float b = dot(localOrigin, localDir);
    const float c = dot(localOrigin, localOrigin) - 12.25f;
    const float disc = b * b - a * c;
    if (disc < 0.0f)
        return false;

    const float sqrtDisc = std::sqrt(disc);
    const float invA = 1.0f / a;
    const float tEntry = (-b - sqrtDisc) * invA;
    const float tExit = (-b + sqrtDisc) * invA;

    if (tExit < tMin || tEntry > tMax)
        return false;

    float tCur = (tEntry < tMin) ? tMin : tEntry;
    const float tEnd = (tExit < tMax) ? tExit : tMax;

    if (tCur >= tEnd)
        return false;

    float prevVal = f(localOrigin + localDir * tCur);
    float stepSize = 0.05f;
    bool hitFound = false;
    float tPrev = tCur;

    while (tCur < tEnd)
    {
        tCur += stepSize;
        if (tCur > tEnd)
            tCur = tEnd;

        Vector3f p = localOrigin + localDir * tCur;
        const float cur = f(p);

        if (prevVal * cur < 0.0f)
        {
            float tRefined = tCur;
            bool newtonConverged = false;

            for (int i = 0; i < 4; ++i)
            {
                const Vector3f pRef = localOrigin + localDir * tRefined;
                const float fVal = f(pRef);
                const Vector3f grad = gradient(pRef);
                const float deriv = dot(grad, localDir);

                if (std::fabs(deriv) < 1e-6f)
                    break;

                const float delta = fVal / deriv;
                tRefined -= delta;

                if (tRefined < tPrev || tRefined > tCur)
                    break;

                if (std::fabs(delta) < 1e-5f)
                {
                    newtonConverged = true;
                    break;
                }
            }

            if (!newtonConverged)
            {
                float ta = tPrev;
                float tb = tCur;
                for (int i = 0; i < 8; ++i)
                {
                    const float tm = (ta + tb) * 0.5f;
                    const Vector3f pm = localOrigin + localDir * tm;
                    if (f(pm) * prevVal < 0.0f)
                        tb = tm;
                    else
                        ta = tm;
                }
                tRefined = (ta + tb) * 0.5f;
            }

            tCur = tRefined;
            hitFound = true;
            break;
        }

        const float absVal = std::fabs(cur);
        if (absVal < 0.5f)
            stepSize = 0.01f;
        else if (absVal < 2.0f)
            stepSize = 0.03f;
        else
            stepSize = 0.05f;

        prevVal = cur;
        tPrev = tCur;
    }

    if (!hitFound)
        return false;

    const Vector3f localHit = localOrigin + localDir * tCur;
    const Vector3f normal = gradient(localHit).unit_vector();

    hit.t = tCur;
    hit.point = ray.at(hit.t);
    hit.normal = normal;
    hit.uv = uvmap::sphere(localHit);
    // hit.color = _colorF;
    if (this->_material)
        hit.material = *this->_material;
    hit.primitive = this;

    return true;
}

bool rc::Tanglecube::isFinite() const
{
    return (true);
}

rc::AABB rc::Tanglecube::bounding_box() const
{
    const float localBoundRadius = 3.5f;
    Vector3f half = Vector3f(_size * localBoundRadius * _scale.x,
                             _size * localBoundRadius * _scale.y,
                             _size * localBoundRadius * _scale.z);
    Vector3f min = _center - half;
    Vector3f max = _center + half;
    return AABB{min, max};
}

const rc::Material *rc::Tanglecube::getMaterial() const
{
    return (this->_material);
}

void rc::Tanglecube::setMaterial(const Material *material)
{
    this->_material = material;
}

bool rc::Tanglecube::isHidden() const
{
    return (this->_hidden);
}

void rc::Tanglecube::setHidden(bool hidden)
{
    this->_hidden = hidden;
}

std::map<std::string, std::pair<std::string, rc::PropertyType>> rc::Tanglecube::getProperties() const
{
    return {
        {"position", {this->_center.toString(), PropertyType::VECTOR3F}},
        {"rotation", {this->_rotation.toString(), PropertyType::VECTOR3F}},
        {"threshold", {std::to_string(this->_threshold), PropertyType::FLOAT}},
        {"size", {std::to_string(this->_size), PropertyType::FLOAT}},
    };
}

void rc::Tanglecube::setPropertyFloat(const std::string &key, float value)
{
    if (key == "threshold")
    {
        this->_threshold = value;
    }
    else if (key == "size")
    {
        this->_size = value;
    }
    else
    {
        std::cerr << "Could not update " << this->_name << " (" << this->getTypeName() << ") property : " << key << " : Unknown property" << std::endl;
    }
}
