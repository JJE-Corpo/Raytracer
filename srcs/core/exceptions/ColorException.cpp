

#include "ColorException.hpp"

rc::ColorException::ColorException(const std::string &message) : _message(message)
{
}

const char *rc::ColorException::what() const noexcept
{
    return (this->_message.c_str());
}
