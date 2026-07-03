//
// Created by jazema on 4/21/26.
//

#include "LoadingSceneException.hpp"

rc::LoadingSceneException::LoadingSceneException(const ExceptionType type, const std::string &message) : _type(type)
{
    if (type == ExceptionType::WRONG_FILE_CONTENT)
        this->_message = "Wrong file content: " + message;
}

const char *rc::LoadingSceneException::what() const noexcept
{
    switch (this->_type)
    {
        case ExceptionType::UNKNOWN_FILE:
            return "Unknown file";
        case ExceptionType::CANNOT_OPEN_FILE:
            return "Cannot open file";
        case ExceptionType::INVALID_FILE_EXTENSION:
            return "Invalid file extension: expected .json";
        case ExceptionType::WRONG_FILE_CONTENT:
            return this->_message.c_str();
        default:
            return "Unknown error";
    }
}
