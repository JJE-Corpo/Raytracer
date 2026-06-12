

#include "VectorException.hpp"

rc::VectorException::VectorException(const std::string &message) : _message(message)
{
}

const char *rc::VectorException::what() const noexcept
{
    return (this->_message.c_str());
}
