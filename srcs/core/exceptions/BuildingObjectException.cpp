//
// Created by jazema on 4/21/26.
//

#include "BuildingObjectException.hpp"

namespace rc
{
    BuildingObjectException::BuildingObjectException(const std::string &message) : _message(message)
    {
    }

    const char *BuildingObjectException::what() const noexcept
    {
        return (this->_message.c_str());
    }
}
