//
// Created by jazema on 4/30/26.
//

#ifndef UTILS_HPP
#define UTILS_HPP
#include <cstdlib>
#include <random>

class Utils
{
    public:

        static double random_double() {
            static std::uniform_real_distribution distribution(0.0, 1.0);
            static std::mt19937 generator;
            return distribution(generator);
        }

        static double random_double(double min, double max)
        {
            return min + (max - min) * random_double();
        }

        static double degrees_to_radians(double degrees) {
            return degrees * 3.14159265358979323846 / 180.0;
        }

        static bool isFloat(const std::string &value)
        {
            try
            {
                size_t pos;
                std::stof(value, &pos);

                return (pos == value.size());
            }
            catch (...)
            {
                return (false);
            }
        }

        static bool isUnsignedLong(const std::string &value)
        {
            try
            {
                size_t pos;
                std::stoul(value, &pos);

                return (pos == value.size());
            }
            catch (...)
            {
                return (false);
            }
        }
};

#endif
