//
// Created by jazema on 4/21/26.
//

#ifndef COLOR_HPP
#define COLOR_HPP
#include <cstdint>
#include <algorithm>
#include <cmath>

#include "../core/exceptions/ColorException.hpp"

namespace rc
{
    struct Color
    {
        uint8_t r;
        uint8_t g;
        uint8_t b;
        uint8_t a;

        Color() : r(0), g(0), b(0), a(255) {}
        Color(const uint8_t _r, const uint8_t _g, const uint8_t _b, const uint8_t _a = 255) : r(_r), g(_g), b(_b), a(_a) {}

        Color(std::string str)
        {
            // parse string of format "{r, g, b}" or "{r, g, b, a}"
            size_t first_brace = str.find('{');
            size_t last_brace = str.find('}');

            if (first_brace == std::string::npos || last_brace == std::string::npos || last_brace <= first_brace)
                throw ColorException("Invalid color string format");

            std::string content = str.substr(first_brace + 1, last_brace - first_brace - 1);
            size_t first_comma = content.find(',');
            size_t second_comma = content.find(',', first_comma + 1);

            if (first_comma == std::string::npos || second_comma == std::string::npos)
                throw ColorException("Invalid color string format");

            try
            {
                this->r = static_cast<uint8_t>(std::stoi(content.substr(0, first_comma)));
                this->g = static_cast<uint8_t>(std::stoi(content.substr(first_comma + 1, second_comma - first_comma - 1)));
                this->b = static_cast<uint8_t>(std::stoi(content.substr(second_comma + 1)));
                this->a = 255;
            }
            catch (const std::exception &e)
            {
                throw ColorException("Invalid color string format" + std::string(e.what()));
            }
        }

        friend Color operator+(const Color &lhs, const Color &rhs)
        {
            auto addc = [](uint8_t x, uint8_t y) -> uint8_t {
                int v = int(x) + int(y);
                return static_cast<uint8_t>(std::min(255, v));
            };
            return Color(addc(lhs.r, rhs.r), addc(lhs.g, rhs.g), addc(lhs.b, rhs.b), addc(lhs.a, rhs.a));
        }

        friend Color operator*(const Color &c, const Color &c2)
        {
            auto mulc = [&](uint8_t v, uint8_t v2) -> uint8_t {
                int r = int(std::lround(v * v2));
                return static_cast<uint8_t>(std::min(255, std::max(0, r)));
            };
            return Color(mulc(c.r, c2.r), mulc(c.g, c2.g), mulc(c.b, c2.b), 255);
        }

        friend Color operator*(const Color &c, double scalar)
        {
            auto mulc = [&](uint8_t v) -> uint8_t {
                int r = int(std::lround(v * scalar));
                return static_cast<uint8_t>(std::min(255, std::max(0, r)));
            };
            return Color(mulc(c.r), mulc(c.g), mulc(c.b), 255);
        }

        friend Color operator*(double scalar, const Color &c)
        {
            return c * scalar;
        }

        Color &operator+=(const Color &color)
        {
            int r = int(this->r) + int(color.r);
            int g = int(this->g) + int(color.g);
            int b = int(this->b) + int(color.b);
            int a = int(this->a) + int(color.a);

            this->r = static_cast<uint8_t>(std::min(255, r));
            this->g = static_cast<uint8_t>(std::min(255, g));
            this->b = static_cast<uint8_t>(std::min(255, b));
            this->a = static_cast<uint8_t>(std::min(255, a));
            return (*this);
        }
    };

    struct ColorF {
        float r;
        float g;
        float b;

        ColorF operator+(const ColorF& o) const
        {
            return {r + o.r, g + o.g, b + o.b};
        }

        ColorF operator*(float s) const
        {
            return {r * s, g * s, b * s};
        }

        ColorF operator*(const ColorF& o) const
        {
            return {r * o.r, g * o.g, b * o.b};
        }

        static ColorF fromColor(const Color& c) {
            return {c.r / 255.f, c.g / 255.f, c.b / 255.f};
        }

        Color toColor() const
        {
            return Color(
                static_cast<uint8_t>(std::clamp(this->r * 255.f, 0.f, 255.f)),
                static_cast<uint8_t>(std::clamp(this->g * 255.f, 0.f, 255.f)),
                static_cast<uint8_t>(std::clamp(this->b * 255.f, 0.f, 255.f))
            );
        }
    };
}

#endif
