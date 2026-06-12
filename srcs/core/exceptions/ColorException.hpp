

#ifndef COLOREXCEPTION_HPP
#define COLOREXCEPTION_HPP

#include <exception>
#include <string>

namespace rc
{
    class ColorException : public std::exception
    {
        public:
            ColorException(const std::string &message);

            const char *what() const noexcept override;
        private:
            std::string _message;
    };
}

#endif
