
#ifndef VECTOREXCEPTION_HPP
#define VECTOREXCEPTION_HPP

#include <exception>
#include <string>

namespace rc
{
    class VectorException : public std::exception
    {
        public:
            VectorException(const std::string &message);

            const char *what() const noexcept override;
        private:
            std::string _message;
    };
}

#endif
