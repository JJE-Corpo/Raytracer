/*
** EPITECH PROJECT, 2025
** raytracer [WSL: Ubuntu-22.04]
** File description:
** Fractal.cpp
*/

#include "Fractal.hpp"
#include <cmath>
#include <iostream>
#include "../../common/Intersection.hpp"
#include "../../common/Ray.hpp"
#include "../../common/AABB.hpp"
#include "../../common/Material.hpp"
#include "../../common/Vector.hpp"
#include "../../common/UvMapping.hpp"
#include "../../core/scene/builder/SceneObjectBuilder.hpp"

rc::Fractal::Fractal(const std::string &name, const Vector3f &center, float size, float power, int iterations, const Material *material, FractalType type) :
    _type(type),
    _center(center),
    _size(size),
    _power(power),
    _iterations(iterations),
    _material(material),
    _bbox{center - Vector3f(size * 1.5f, size * 1.5f, size * 1.5f),
          center + Vector3f(size * 1.5f, size * 1.5f, size * 1.5f)}
{
    float bboxScale = 1.5f;
    switch (type) {
        case FractalType::MANDELBOX:     bboxScale = 6.0f; break;
        case FractalType::SIERPINSKI:    bboxScale = 2.0f; break;
        case FractalType::MENGER_SPONGE: bboxScale = 1.5f; break;
        case FractalType::JULIA3D:       bboxScale = 1.5f; break;
        case FractalType::MANDELBULB:    bboxScale = 1.5f; break;
    }
    this->_bbox = AABB{
        center - Vector3f(size * bboxScale, size * bboxScale, size * bboxScale),
        center + Vector3f(size * bboxScale, size * bboxScale, size * bboxScale)
    };

    if (!name.empty())
        this->_name = name;

    // std::cout << "Fractal constructed" << std::endl;
}

bool rc::Fractal::isHidden() const
{
    return this->_hidden;
}

void rc::Fractal::setHidden(bool hidden)
{
    this->_hidden = hidden;
}

float rc::Fractal::deMandelbulb(const Vector3f &pos) const
{
    Vector3f z = pos;
    float dr = 1.0f;
    float r = std::sqrt(z.x * z.x + z.y * z.y + z.z * z.z);

    if (r < 1e-6f)
        return 0.0f;

    for (int i = 0; i < this->_iterations; ++i) {
        r = std::sqrt(z.x * z.x + z.y * z.y + z.z * z.z);

        if (r > this->_bailout)
            break;

        float theta = std::acos(z.z / r);
        float phi = std::atan2(z.y, z.x);

        dr = std::pow(r, this->_power - 1.0f) * this->_power * dr + 1.0f;

        float zr = std::pow(r, this->_power);
        theta = theta * this->_power;
        phi = phi * this->_power;

        float sinTheta = std::sin(theta);
        z.x = zr * sinTheta * std::cos(phi);
        z.y = zr * sinTheta * std::sin(phi);
        z.z = zr * std::cos(theta);

        z.x += pos.x;
        z.y += pos.y;
        z.z += pos.z;
    }

    return 0.5f * std::log(r) * r / dr;
}

float rc::Fractal::deMandelbox(const Vector3f &pos) const
{
    Vector3f z = pos;
    float dr = 1.0f;
    const float L = this->_foldingLimit;
    const float minR2 = this->_mandelboxMinRadius * this->_mandelboxMinRadius;
    const float fixedR2 = this->_mandelboxFixedRadius * this->_mandelboxFixedRadius;
    const float s = this->_mandelboxScale;

    for (int i = 0; i < this->_iterations; ++i) {
        // Box fold
        if (z.x >  L) z.x =  2.0f * L - z.x;
        else if (z.x < -L) z.x = -2.0f * L - z.x;
        if (z.y >  L) z.y =  2.0f * L - z.y;
        else if (z.y < -L) z.y = -2.0f * L - z.y;
        if (z.z >  L) z.z =  2.0f * L - z.z;
        else if (z.z < -L) z.z = -2.0f * L - z.z;

        // Sphere fold
        float r2 = z.x * z.x + z.y * z.y + z.z * z.z;
        if (r2 < minR2) {
            float f = fixedR2 / minR2;
            z = z * f;
            dr *= f;
        } else if (r2 < fixedR2) {
            float f = fixedR2 / r2;
            z = z * f;
            dr *= f;
        }

        // Scale + translate
        z = z * s + pos;
        dr = dr * std::abs(s) + 1.0f;
    }

    float r = std::sqrt(z.x * z.x + z.y * z.y + z.z * z.z);
    return r / std::abs(dr);
}

float rc::Fractal::deJulia(const Vector3f &pos) const
{
    Vector3f z = pos;
    float dr = 1.0f;
    float r = std::sqrt(z.x * z.x + z.y * z.y + z.z * z.z);

    if (r < 1e-6f)
        return 0.0f;

    for (int i = 0; i < this->_iterations; ++i) {
        r = std::sqrt(z.x * z.x + z.y * z.y + z.z * z.z);
        if (r > this->_bailout)
            break;

        float theta = std::acos(z.z / r);
        float phi = std::atan2(z.y, z.x);

        dr = std::pow(r, this->_power - 1.0f) * this->_power * dr + 1.0f;

        float zr = std::pow(r, this->_power);
        theta *= this->_power;
        phi *= this->_power;

        float sinTheta = std::sin(theta);
        z.x = zr * sinTheta * std::cos(phi) + this->_juliaC.x;
        z.y = zr * sinTheta * std::sin(phi) + this->_juliaC.y;
        z.z = zr * std::cos(theta)          + this->_juliaC.z;
    }

    return 0.5f * std::log(r) * r / dr;
}

float rc::Fractal::deMengerSponge(const Vector3f &pos) const
{
    Vector3f z = pos;
    float scale = 1.0f;

    for (int i = 0; i < this->_iterations; ++i) {
        z.x = std::abs(z.x);
        z.y = std::abs(z.y);
        z.z = std::abs(z.z);

        if (z.x < z.y) std::swap(z.x, z.y);
        if (z.x < z.z) std::swap(z.x, z.z);
        if (z.y < z.z) std::swap(z.y, z.z);

        z = z * 3.0f - Vector3f(2.0f, 2.0f, 2.0f);
        if (z.z < -1.0f) z.z += 2.0f;
        scale *= 3.0f;
    }

    // Distance signée à un cube unité
    float dx = std::abs(z.x) - 1.0f;
    float dy = std::abs(z.y) - 1.0f;
    float dz = std::abs(z.z) - 1.0f;

    float maxD = std::max(dx, std::max(dy, dz));
    float ox = std::max(dx, 0.0f);
    float oy = std::max(dy, 0.0f);
    float oz = std::max(dz, 0.0f);
    float outsideD = std::sqrt(ox * ox + oy * oy + oz * oz);

    return (std::min(maxD, 0.0f) + outsideD) / scale;
}

float rc::Fractal::deSierpinski(const Vector3f &pos) const
{
    Vector3f z = pos;
    const float s = this->_sierpinskiScale;

    for (int i = 0; i < this->_iterations; ++i) {
        if (z.x + z.y < 0.0f) { float t = -z.y; z.y = -z.x; z.x = t; }
        if (z.x + z.z < 0.0f) { float t = -z.z; z.z = -z.x; z.x = t; }
        if (z.y + z.z < 0.0f) { float t = -z.z; z.z = -z.y; z.y = t; }
        z = z * s - Vector3f(s - 1.0f, s - 1.0f, s - 1.0f);
    }

    float r = std::sqrt(z.x * z.x + z.y * z.y + z.z * z.z);
    return r * std::pow(s, -float(this->_iterations));
}

float rc::Fractal::distanceEstimator(const Vector3f &pos) const
{
    switch (this->_type) {
        case FractalType::MANDELBULB:    return deMandelbulb(pos);
        case FractalType::MANDELBOX:     return deMandelbox(pos);
        case FractalType::JULIA3D:       return deJulia(pos);
        case FractalType::MENGER_SPONGE: return deMengerSponge(pos);
        case FractalType::SIERPINSKI:    return deSierpinski(pos);
    }
    return 0.0f;
}

bool rc::Fractal::intersect(const Ray &ray, float tMin, float tMax, Intersection &hit) const
{
    float tEnter = tMin;
    float tExit = tMax;

    for (int axis = 0; axis < 3; ++axis) {
        float invD = 1.0f / ray.direction[axis];
        float t0 = (this->_bbox.min[axis] - ray.origin[axis]) * invD;
        float t1 = (this->_bbox.max[axis] - ray.origin[axis]) * invD;

        if (invD < 0.0f)
            std::swap(t0, t1);

        tEnter = (t0 > tEnter) ? t0 : tEnter;
        tExit = (t1 < tExit) ? t1 : tExit;

        if (tExit <= tEnter)
            return false;
    }

    const float invSize = 1.0f / this->_size;
    float tCur = (tEnter > tMin) ? tEnter : tMin;
    const float maxDistance = (tExit < tMax) ? tExit : tMax;

    bool hitFound = false;

    for (int step = 0; step < this->_maxRaySteps; ++step) {
        Vector3f worldP = ray.origin + ray.direction * tCur;
        Vector3f localP = (worldP - this->_center) * invSize;

        float d = distanceEstimator(localP);
        float dWorld = d * this->_size;

        if (dWorld < this->_epsilon) {
            hitFound = true;
            break;
        }

        tCur += dWorld;

        if (tCur > maxDistance)
            return false;
    }

    if (!hitFound)
        return false;

    Vector3f worldHit = ray.origin + ray.direction * tCur;
    const Vector3f localHit = (worldHit - this->_center) * invSize;
    const float h = 0.0001f;

    const Vector3f d1(1.0f, -1.0f, -1.0f);
    const Vector3f d2(-1.0f, -1.0f, 1.0f);
    const Vector3f d3(-1.0f, 1.0f, -1.0f);
    const Vector3f d4(1.0f, 1.0f, 1.0f);

    Vector3f n1 = d1 * distanceEstimator(localHit + d1 * h);
    Vector3f n2 = d2 * distanceEstimator(localHit + d2 * h);
    Vector3f n3 = d3 * distanceEstimator(localHit + d3 * h);
    Vector3f n4 = d4 * distanceEstimator(localHit + d4 * h);

    Vector3f normal = (n1 + n2 + n3 + n4).unit_vector();

    hit.t = tCur;
    hit.point = worldHit;
    hit.normal = normal;
    hit.uv = uvmap::sphere(localHit);
    if (this->_material)
        hit.material = *this->_material;
    hit.primitive = this;

    return (true);
}

bool rc::Fractal::isFinite() const
{
    return (true);
}

rc::AABB rc::Fractal::bounding_box() const
{
    return (this->_bbox);
}

std::string rc::Fractal::getName() const
{
    return (this->_name);
}

std::string rc::Fractal::getTypeName() const
{
    return (PRIMITIVE_FRACTAL);
}

std::map<std::string, std::pair<std::string, rc::PropertyType>> rc::Fractal::getProperties() const
{
    std::string typeStr;
    switch (this->_type) {
        case FractalType::MANDELBULB:    typeStr = "mandelbulb"; break;
        case FractalType::MANDELBOX:     typeStr = "mandelbox"; break;
        case FractalType::JULIA3D:       typeStr = "julia3d"; break;
        case FractalType::MENGER_SPONGE: typeStr = "menger"; break;
        case FractalType::SIERPINSKI:    typeStr = "sierpinski"; break;
    }

    return {
        {"fractal_type", {typeStr, PropertyType::STRING}},
        {"position", {this->_center.toString(), PropertyType::VECTOR3F}},
        {"rotation", {this->_rotation.toString(), PropertyType::VECTOR3F}},
        {"scale", {this->_scale.toString(), PropertyType::VECTOR3F}},
        {"size", {std::to_string(this->_size), PropertyType::FLOAT}},
        {"power", {std::to_string(this->_power), PropertyType::FLOAT}},
        {"iterations", {std::to_string(this->_iterations), PropertyType::INT}},
    };
}

void rc::Fractal::setPropertyFloat(const std::string &key, float value)
{
    if (key == "size")
        this->_size = value;
    if (key == "power")
        this->_power = value;
}

rc::Vector3f rc::Fractal::getPosition() const
{
    return this->_center;
}

rc::Vector3f rc::Fractal::getRotation() const
{
    return this->_rotation;
}

rc::Vector3f rc::Fractal::getScale() const
{
    return this->_scale;
}

void rc::Fractal::setPosition(const Vector3f &position)
{
    // The culling bbox is stored in world space relative to _center, so it must
    // move with the center (e.g. when a parent group translates this fractal),
    // otherwise intersect() culls the ray against a stale box and the fractal
    // renders clipped or vanishes.
    const Vector3f delta = position - this->_center;
    this->_bbox = AABB{this->_bbox.min + delta, this->_bbox.max + delta};
    this->_center = position;
}

void rc::Fractal::setRotation(const Vector3f &rotation)
{
    this->_rotation = rotation;
}

void rc::Fractal::setScale(const Vector3f &scale)
{
    this->_scale = scale;
}

const rc::Material *rc::Fractal::getMaterial() const
{
    return (this->_material);
}

void rc::Fractal::setMaterial(const Material *material)
{
    this->_material = material;
}

