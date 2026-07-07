//
// Created by jazema on 4/21/26.
//

#ifndef VECTOR_HPP
#define VECTOR_HPP
#include <cmath>
#include <stdexcept>

#include "Utils.hpp"
#include "../core/exceptions/VectorException.hpp"
// sources: https://iquilezles.org/articles/intersectors/

namespace rc
{
    typedef struct Vector2f
    {
        float x;
        float y;

        Vector2f() : x(0), y(0)
        {
        }

        Vector2f(float x, float y) : x(x), y(y)
        {
        }

        Vector2f(std::string str)
        {
            // parse string of format "{x, y}"
            size_t first_brace = str.find('{');
            size_t last_brace = str.find('}');

            if (first_brace == std::string::npos || last_brace == std::string::npos || last_brace <= first_brace)
                throw VectorException("Invalid vector string format");

            std::string content = str.substr(first_brace + 1, last_brace - first_brace - 1);
            size_t comma = content.find(',');

            if (comma == std::string::npos)
                throw VectorException("Invalid vector string format");

            try
            {
                this->x = std::stof(content.substr(0, comma));
                this->y = std::stof(content.substr(comma + 1));
            }
            catch (const std::exception &e)
            {
                throw VectorException("Invalid vector string format" + std::string(e.what()));
            }
        }

        bool operator==(const Vector2f &v) const
        {
            return (this->x == v.x && this->y == v.y);
        }

        bool operator!=(const Vector2f &v) const
        {
            return !(*this == v);
        }
    } Vector2f;

    typedef struct Vector3f
    {
        float x;
        float y;
        float z;

        Vector3f() : x(0.0f), y(0.0f), z(0.0f)
        {
        }

        Vector3f(float x, float y, float z) : x(x), y(y), z(z)
        {
        }

        Vector3f(std::string str)
        {
            // parse string of format "{x, y, z}"
            size_t first_brace = str.find('{');
            size_t last_brace = str.find('}');

            if (first_brace == std::string::npos || last_brace == std::string::npos || last_brace <= first_brace)
                throw VectorException("Invalid vector string format");

            std::string content = str.substr(first_brace + 1, last_brace - first_brace - 1);
            size_t first_comma = content.find(',');
            size_t last_comma = content.rfind(',');

            if (first_comma == std::string::npos || last_comma == std::string::npos || last_comma <= first_comma)
                throw VectorException("Invalid vector string format");

            try
            {
                this->x = std::stof(content.substr(0, first_comma));
                this->y = std::stof(content.substr(first_comma + 1, last_comma - first_comma - 1));
                this->z = std::stof(content.substr(last_comma + 1));
            }
            catch (const std::exception &e)
            {
                throw VectorException("Invalid vector string format" + std::string(e.what()));
            }
        }

        double length() const
        {
            return std::sqrt(length_squared());
        }

        double length_squared() const
        {
            return this->x * this->x + this->y * this->y + this->z * this->z;
        }

        Vector3f operator+(const Vector3f &v) const
        {
            return {x + v.x, y + v.y, z + v.z};
        }

        Vector3f operator-(const Vector3f &v) const
        {
            return {x - v.x, y - v.y, z - v.z};
        }

        Vector3f operator-() const
        {
            return {-x, -y, -z};
        }

        Vector3f operator*(const float s) const
        {
            return {x * s, y * s, z * s};
        }

        Vector3f operator/(double t) const
        {
            return *this * (1/t);
        }

        bool operator==(const Vector3f &v) const
        {
            return (this->x == v.x && this->y == v.y && this->z == v.z);
        }

        bool operator!=(const Vector3f &v) const
        {
            return !(*this == v);
        }

        float operator[](size_t index) const
        {
            if (index == 0)
                return (this->x);
            if (index == 1)
                return (this->y);
            if (index == 2)
                return (this->z);
            throw std::out_of_range("index out of range");
        }

        Vector3f unit_vector() const
        {
            return *this / this->length();
        }

        Vector3f cross(const Vector3f &b) const
        {
            return Vector3f{
                this->y * b.z - this->z * b.y,
                this->z * b.x - this->x * b.z,
                this->x * b.y - this->y * b.x
            };
        }

        std::string toString() const
        {
            return ("{" + std::to_string(this->x) + ", " + std::to_string(this->y) + ", " + std::to_string(this->z) + "}");
        }

        static Vector3f random()
        {
            return Vector3f(Utils::random_double(), Utils::random_double(), Utils::random_double());
        }

        static Vector3f random(double min, double max)
        {
            return Vector3f(Utils::random_double(min,max), Utils::random_double(min,max), Utils::random_double(min,max));
        }
    } Vector3f;

    inline Vector3f operator*(const float s, const Vector3f &v)
    {
        return (v * s);
    }

    inline float dot(const Vector3f &a, const Vector3f &b)
    {
        return (a.x * b.x + a.y * b.y + a.z * b.z);
    }

    inline float length(const Vector3f& v)
    {
        return std::sqrt(v.x * v.x + v.y * v.y + v.z * v.z);
    }

    inline Vector3f normalize(const Vector3f& v)
    {
        float len = length(v);
        return v * (1.0f / len);
    }

    inline Vector3f rotate(const Vector3f& v, const Vector3f& rot)
    {
        float cx = std::cos(rot.x), sx = std::sin(rot.x);
        float cy = std::cos(rot.y), sy = std::sin(rot.y);
        float cz = std::cos(rot.z), sz = std::sin(rot.z);

        Vector3f vx = {
            v.x,
            v.y * cx - v.z * sx,
            v.y * sx + v.z * cx
        };

        Vector3f vy = {
            vx.x * cy + vx.z * sy,
            vx.y,
            -vx.x * sy + vx.z * cy
        };

        Vector3f vz = {
            vy.x * cz - vy.y * sz,
            vy.x * sz + vy.y * cz,
            vy.z
        };

        return (vz);
    }

    inline Vector3f degToRad(const Vector3f &deg)
    {
        return {
            static_cast<float>(Utils::degrees_to_radians(deg.x)),
            static_cast<float>(Utils::degrees_to_radians(deg.y)),
            static_cast<float>(Utils::degrees_to_radians(deg.z))
        };
    }

    inline Vector3f inverseRotate(const Vector3f &v, const Vector3f &rot)
    {
        float cx = std::cos(rot.x), sx = std::sin(rot.x);
        float cy = std::cos(rot.y), sy = std::sin(rot.y);
        float cz = std::cos(rot.z), sz = std::sin(rot.z);

        Vector3f a = {v.x * cz + v.y * sz, -v.x * sz + v.y * cz, v.z};   // Rz(-z)
        Vector3f b = {a.x * cy - a.z * sy, a.y, a.x * sy + a.z * cy};    // Ry(-y)
        return {b.x, b.y * cx + b.z * sx, -b.y * sx + b.z * cx};         // Rx(-x)
    }

    typedef struct Vector2i
    {
        int x;
        int y;

        Vector2i() : x(0), y(0)
        {
        }

        Vector2i(int x, int y) : x(x), y(y)
        {
        }

        Vector2i(std::string str)
        {
            // parse string of format "{x, y}"
            size_t first_brace = str.find('{');
            size_t last_brace = str.find('}');

            if (first_brace == std::string::npos || last_brace == std::string::npos || last_brace <= first_brace)
                throw VectorException("Invalid vector string format");
            std::string content = str.substr(first_brace + 1, last_brace - first_brace - 1);
            size_t comma = content.find(',');

            if (comma == std::string::npos)
                throw VectorException("Invalid vector string format");

            try
            {
                this->x = std::stoi(content.substr(0, comma));
                this->y = std::stoi(content.substr(comma + 1));
            }
            catch (const std::exception &e)
            {
                throw VectorException("Invalid vector string format" + std::string(e.what()));
            }
        }

        bool operator==(const Vector2i &v) const
        {
            return (this->x == v.x && this->y == v.y);
        }

        bool operator!=(const Vector2i &v) const
        {
            return !(*this == v);
        }

    } Vector2i;

    typedef struct Vector3i
    {
        int x;
        int y;
        int z;

        Vector3i() : x(0), y(0), z(0)
        {
        }

        Vector3i(int x, int y, int z) : x(x), y(y), z(z)
        {
        }

        Vector3i(std::string str)
        {
            // parse string of format "{x, y, z}"
            size_t first_brace = str.find('{');
            size_t last_brace = str.find('}');

            if (first_brace == std::string::npos || last_brace == std::string::npos || last_brace <= first_brace)
                throw VectorException("Invalid vector string format");
            std::string content = str.substr(first_brace + 1, last_brace - first_brace - 1);
            size_t first_comma = content.find(',');
            size_t last_comma = content.rfind(',');

            if (first_comma == std::string::npos || last_comma == std::string::npos || last_comma <= first_comma)
                throw VectorException("Invalid vector string format");

            try
            {
                this->x = std::stoi(content.substr(0, first_comma));
                this->y = std::stoi(content.substr(first_comma + 1, last_comma - first_comma - 1));
                this->z = std::stoi(content.substr(last_comma + 1));
            }
            catch (const std::exception &e)
            {
                throw VectorException("Invalid vector string format" + std::string(e.what()));
            }
        }

        bool operator==(const Vector3i &v) const
        {
            return (this->x == v.x && this->y == v.y && this->z == v.z);
        }

        bool operator!=(const Vector3i &v) const
        {
            return !(*this == v);
        }
    } Vector3i;
}

#endif
