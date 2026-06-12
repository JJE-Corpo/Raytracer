/*
* ____  _ _____   ___  _   _   _   _  _         ___ _____ _  _   _   _  _
* |_ _| _ |_   _|/ _ \| | | | /_\ | \| |   <3  | __|_   _| || | /_\ | \| |
*  | | | |  | | | (_) | |_| |/ _ \| .` |   <3  | _|  | | | __ |/ _ \| .` |
*  |_| |_|  |_|  \___/ \___//_/ \_\_|\_|   <3  |___| |_| |_||_/_/ \_\_|\_|
*
* -----------------------------------------------------------------------
* File:    Matrix.hpp
* Who:     Titouan & Ethan
* Date:    2026-05-10
* -----------------------------------------------------------------------
*/


#ifndef MATRIX_HPP
#define MATRIX_HPP

#include "Vector.hpp"

namespace rc
{
    template <size_t N>
    struct Matrix
    {
        float data[N][N];

        Matrix<N> operator*(const Matrix<N> &other) const
        {
            Matrix<N> result;

            for (size_t i = 0; i < N; ++i)
            {
                for (size_t j = 0; j < N; ++j)
                {
                    result.data[i][j] = 0.0f;
                    for (size_t k = 0; k < N; ++k)
                    {
                        result.data[i][j] += this->data[i][k] * other.data[k][j];
                    }
                }
            }

            return result;
        }

        Vector3f operator*(const Vector3f &v) const
        {
            float x = data[0][0] * v.x + data[0][1] * v.y + data[0][2] * v.z + data[0][3];
            float y = data[1][0] * v.x + data[1][1] * v.y + data[1][2] * v.z + data[1][3];
            float z = data[2][0] * v.x + data[2][1] * v.y + data[2][2] * v.z + data[2][3];
            float w = data[3][0] * v.x + data[3][1] * v.y + data[3][2] * v.z + data[3][3];

            if (w != 0.0f)
            {
                x /= w;
                y /= w;
                z /= w;
            }

            return Vector3f(x, y, z);
        }

        static Matrix<N> identity()
        {
            Matrix<N> result;
            for (size_t i = 0; i < N; ++i)
            {
                for (size_t j = 0; j < N; ++j)
                {
                    result.data[i][j] = (i == j) ? 1.0f : 0.0f;
                }
            }
            return result;
        }

        static Matrix<N> translation(float tx, float ty, float tz)
        {
            Matrix<N> result = identity();
            result.data[0][3] = tx;
            result.data[1][3] = ty;
            result.data[2][3] = tz;
            return result;
        }

        static Matrix<N> rotation_x(float angle)
        {
            Matrix<N> result = identity();
            float c = std::cos(angle);
            float s = std::sin(angle);
            result.data[1][1] = c;
            result.data[1][2] = -s;
            result.data[2][1] = s;
            result.data[2][2] = c;
            return result;
        }

        static Matrix<N> rotation_y(float angle)
        {
            Matrix<N> result = identity();
            float c = std::cos(angle);
            float s = std::sin(angle);
            result.data[0][0] = c;
            result.data[0][2] = -s;
            result.data[2][0] = +s;
            result.data[2][2] = c;
            return result;
        }

        static Matrix<N> rotation_z(float angle)
        {
            Matrix<N> result = identity();
            float c = std::cos(angle);
            float s = std::sin(angle);
            result.data[0][0] = c;
            result.data[0][1] = -s;
            result.data[1][0] = s;
            result.data[1][1] = c;
            return result;
        }

        static Matrix<N> scaling(float sx, float sy, float sz)
        {
            Matrix<N> result = identity();
            result.data[0][0] = sx;
            result.data[1][1] = sy;
            result.data[2][2] = sz;
            return result;
        }
    };
}

#endif
