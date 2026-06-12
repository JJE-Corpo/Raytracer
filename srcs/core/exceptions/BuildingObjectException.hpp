//
// Created by jazema on 4/21/26.
//

#ifndef BUILDINGOBJECTEXCEPTION_HPP
#define BUILDINGOBJECTEXCEPTION_HPP
#include <exception>
#include <string>

namespace rc
{
    class BuildingObjectException : public std::exception
    {
        private:
            std::string _message;
        public:
            BuildingObjectException(const std::string &message);

            const char *what() const noexcept override;
    };
}

#endif
