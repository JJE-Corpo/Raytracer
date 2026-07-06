/*
 * ____  _ _____   ___  _   _   _   _  _         ___ _____ _  _   _   _  _
 * |_ _| _ |_   _|/ _ \| | | | /_\ | \| |   <3  | __|_   _| || | /_\ | \| |
 *  | | | |  | | | (_) | |_| |/ _ \| .` |   <3  | _|  | | | __ |/ _ \| .` |
 *  |_| |_|  |_|  \___/ \___//_/ \_\_|\_|   <3  |___| |_| |_||_/_/ \_\_|\_|
 *
 * -----------------------------------------------------------------------
 * File:    Torus.cpp
 * Who:     Titouan & Ethan
 * Date:    5/6/26
 * -----------------------------------------------------------------------
 */

#include "Torus.hpp"
#include "../../common/Intersection.hpp"
#include "../../common/Ray.hpp"
#include "../../common/Utils.hpp"
#include "iostream"
#include "../../core/scene/builder/SceneObjectBuilder.hpp"
// sources : https://iquilezles.org/articles/intersectors/

namespace rc
{
    Torus::Torus(std::string name, const Vector3f &center, const Vector3f &rotation, float radius, float height, const Material *material):
        _center(center), _rotation(- rotation), _radius(radius), _height(height), _material(material)
    {
        if (!name.empty())
            this->_name = name;

        // std::cout << "Torus constructed" << std::endl;
    }

    std::string Torus::getName() const
    {
        return (this->_name);
    }

    std::string Torus::getTypeName() const
    {
        return PRIMITIVE_TORUS;
    }

    Vector3f Torus::getPosition() const
    {
        return (this->_center);
    }

    Vector3f Torus::getRotation() const
    {
        return (this->_rotation);
    }

    Vector3f Torus::getScale() const
    {
        return this->_scale;
    }

    void Torus::setPosition(const Vector3f &position)
    {
        this->_center = position;
    }

    void Torus::setRotation(const Vector3f &rotation)
    {
        this->_rotation = rotation;
    }

    void Torus::setScale(const Vector3f &scale)
    {
        this->_scale = scale;
    }
int solveCubic(double c[4], double s[3]) {
        int i, num;
        double sub, A, B, C, sq_A, p, q, cb_p, D;
        A = c[2] / c[3]; B = c[1] / c[3]; C = c[0] / c[3];
        sq_A = A * A; p = 1.0/3 * (- 1.0/3 * sq_A + B); q = 1.0/2 * (2.0/27 * A * sq_A - 1.0/3 * A * B + C);
        cb_p = p * p * p; D = q * q + cb_p;
        if (IS_ZERO(D)) {
            if (IS_ZERO(q)) { s[0] = 0; num = 1; }
            else { double u = std::cbrt(-q); s[0] = 2 * u; s[1] = -u; num = 2; }
        } else if (D < 0) {
            double phi = 1.0/3 * std::acos(-q / std::sqrt(-cb_p));
            double t = 2 * std::sqrt(-p);
            s[0] = t * std::cos(phi); s[1] = -t * std::cos(phi + M_PI / 3); s[2] = -t * std::cos(phi - M_PI / 3); num = 3;
        } else {
            double sqrt_D = std::sqrt(D);
            double u = std::cbrt(sqrt_D - q); double v = - std::cbrt(sqrt_D + q);
            s[0] = u + v; num = 1;
        }
        sub = 1.0/3 * A;
        for (i = 0; i < num; ++i) s[i] -= sub;
        return num;
    }

    int solveQuartic(double c[5], double s[4]) {
        double coeffs[4], z, u, v, sub, A, B, C, D, sq_A, p, q, r;
        int i, num;
        A = c[3] / c[4]; B = c[2] / c[4]; C = c[1] / c[4]; D = c[0] / c[4];
        sq_A = A * A; p = -3.0/8 * sq_A + B; q = 1.0/8 * sq_A * A - 1.0/2 * A * B + C;
        r = -3.0/256 * sq_A * sq_A + 1.0/16 * sq_A * B - 1.0/4 * A * C + D;
        if (IS_ZERO(r)) {
            coeffs[0] = q; coeffs[1] = p; coeffs[2] = 0; coeffs[3] = 1;
            num = solveCubic(coeffs, s); s[num++] = 0;
        } else {
            coeffs[0] = 1.0/2 * r * p - 1.0/8 * q * q; coeffs[1] = -r; coeffs[2] = -1.0/2 * p; coeffs[3] = 1;
            solveCubic(coeffs, s); z = s[0];
            u = z * z - r; v = 2 * z - p;
            if (IS_ZERO(u)) u = 0; else if (u > 0) u = std::sqrt(u); else return 0;
            if (IS_ZERO(v)) v = 0; else if (v > 0) v = std::sqrt(v); else return 0;
            coeffs[0] = z - u; coeffs[1] = q < 0 ? -v : v; coeffs[2] = 1;
            num = 0;
            double disc = coeffs[1]*coeffs[1] - 4*coeffs[0];
            if (disc >= 0) { disc = std::sqrt(disc); s[num++] = (-coeffs[1]+disc)/2; s[num++] = (-coeffs[1]-disc)/2; }
            coeffs[0] = z + u; coeffs[1] = q < 0 ? v : -v; coeffs[2] = 1;
            disc = coeffs[1]*coeffs[1] - 4*coeffs[0];
            if (disc >= 0) { disc = std::sqrt(disc); s[num++] = (-coeffs[1]+disc)/2; s[num++] = (-coeffs[1]-disc)/2; }
        }
        sub = 1.0/4 * A;
        for (i = 0; i < num; ++i) s[i] -= sub;
        return num;
    }

    bool Torus::intersect(const Ray &ray, float tMin, float tMax, Intersection &hit) const
    {
        Vector3f P = rotate(ray.origin - _center, _rotation);
        Vector3f U = rotate(ray.direction, _rotation);

        double R = _radius;
        double r = _height;

        double R2 = R * R;
        double r2 = r * r;

        double K = dot(P, P) - R2 - r2;
        double m = dot(P, U);
        double n = dot(U, U);

        double c[5];
        c[4] = n * n;
        c[3] = 4.0 * n * m;
        c[2] = 2.0 * n * K + 4.0 * m * m + 4.0 * R2 * U.z * U.z;
        c[1] = 4.0 * K * m + 8.0 * R2 * P.z * U.z;
        c[0] = K * K + 4.0 * R2 * P.z * P.z - 4.0 * R2 * r2;

        double roots[4];
        int numRoots = solveQuartic(c, roots);

        float t_final = -1.0f;
        float min_t = tMax;

        for (int i = 0; i < numRoots; i++) {
            if (roots[i] > tMin && roots[i] < min_t) {
                min_t = (float)roots[i];
                t_final = min_t;
            }
        }

        if (t_final < 0.0f) return false;

        hit.t = t_final;
        hit.point = ray.origin + ray.direction * t_final;

        Vector3f localP = P + U * t_final;
        float param_k = dot(localP, localP) - R2 - r2;
        Vector3f N;
        N.x = 4.0f * localP.x * param_k;
        N.y = 4.0f * localP.y * param_k;
        N.z = 4.0f * localP.z * (param_k + 2.0f * R2);
        N = normalize(N);

        Vector3f forward_rot(-_rotation.x, -_rotation.y, -_rotation.z);
        N = normalize(rotate(N, forward_rot));
        hit.set_face_normal(ray, N);

        // Toroidal UV: angle around the main axis (u) and around the tube (v).
        constexpr float PI = 3.14159265358979323846f;
        float ringAngle = std::atan2(static_cast<float>(localP.y), static_cast<float>(localP.x));
        float inPlane = std::sqrt(static_cast<float>(localP.x * localP.x + localP.y * localP.y)) - static_cast<float>(R);
        float tubeAngle = std::atan2(static_cast<float>(localP.z), inPlane);
        hit.uv = Vector2f(0.5f + ringAngle / (2.0f * PI), 0.5f + tubeAngle / (2.0f * PI));

        //  hit.color = this->_colorF;
        if (this->_material)
            hit.material = *this->_material;
        hit.primitive = this;

        return true;
    }

    AABB Torus::bounding_box() const
    {
        float outerLimit = _radius + _height;
        Vector3f forward_rot(-_rotation.x, -_rotation.y, -_rotation.z);

        float xs[2] = {-outerLimit, outerLimit};
        float ys[2] = {-outerLimit, outerLimit};
        float zs[2] = {-_height, _height};

        Vector3f minP(std::numeric_limits<float>::max(), std::numeric_limits<float>::max(), std::numeric_limits<float>::max());
        Vector3f maxP(-std::numeric_limits<float>::max(), -std::numeric_limits<float>::max(), -std::numeric_limits<float>::max());

        for (float x : xs) for (float y : ys) for (float z : zs) {
            Vector3f corner = rotate(Vector3f(x, y, z), forward_rot) + _center;
            minP.x = std::min(minP.x, corner.x); minP.y = std::min(minP.y, corner.y); minP.z = std::min(minP.z, corner.z);
            maxP.x = std::max(maxP.x, corner.x); maxP.y = std::max(maxP.y, corner.y); maxP.z = std::max(maxP.z, corner.z);
        }

        return AABB{minP, maxP};
    }
    bool Torus::isFinite() const
    {
        return (true);
    }

    std::map<std::string, std::pair<std::string, PropertyType>> Torus::getProperties() const
    {
        return
        {
            {"position", {"{" + std::to_string(this->_center.x) + ", " + std::to_string(this->_center.y) + ", " + std::to_string(this->_center.z) + "}", PropertyType::VECTOR3F}},
            {"rotation", {"{" + std::to_string(this->_rotation.x) + ", " + std::to_string(this->_rotation.y) + ", " + std::to_string(this->_rotation.z) + "}", PropertyType::VECTOR3F}},
            {"radius", {std::to_string(this->_radius), PropertyType::FLOAT}},
            {"height", {std::to_string(this->_height), PropertyType::FLOAT}},
            {"color", {"{" + std::to_string(this->_colorF.r) + ", " + std::to_string(this->_colorF.g) + ", " + std::to_string(this->_colorF.b) + "}", PropertyType::COLOR}}
        };
    }

    const Material *Torus::getMaterial() const
    {
        return this->_material;
    }

    void Torus::setMaterial(const Material *material)
    {
        this->_material = material;
    }

    void Torus::setPropertyFloat(const std::string &key, float value)
    {
        if (key == "radius")
            this->_radius = value;
        else if (key == "height")
            this->_height = value;
        else
        {
            std::cerr << "Could not update " << this->_name << " (" << this->getTypeName() << ") property : " << key << " : Unknown property" << std::endl;
        }
    }

    bool Torus::isHidden() const
    {
        return this->_hidden;
    }

    void Torus::setHidden(bool hidden)
    {
        this->_hidden = hidden;
    }
}
