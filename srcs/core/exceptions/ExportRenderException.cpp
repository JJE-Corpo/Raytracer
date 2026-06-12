//
// Created by jazema on 4/24/26.
//

#include "ExportRenderException.hpp"

rc::ExportRenderException::ExportRenderException(const std::string &fileName, const std::string &reason)
{
    this->_message = ("Failed to save file '" + fileName + "': " + reason);
}

const char *rc::ExportRenderException::what() const noexcept
{
    return (this->_message.c_str());
}
