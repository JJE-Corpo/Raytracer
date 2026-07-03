//
// Created by jazema on 4/21/26.
//

#ifndef LOADINGSCENEEXCEPTION_HPP
#define LOADINGSCENEEXCEPTION_HPP

#include <exception>
#include <string>

namespace rc
{
    class LoadingSceneException : public std::exception
    {
        public:
            enum class ExceptionType
            {
                UNKNOWN_FILE,
                CANNOT_OPEN_FILE,
                INVALID_FILE_EXTENSION,
                WRONG_FILE_CONTENT,
                OTHER
            };
            LoadingSceneException(ExceptionType type, const std::string& message = "");

            const char *what() const noexcept override;
        private:
            ExceptionType _type;
            std::string _message;
    };
}

#endif
