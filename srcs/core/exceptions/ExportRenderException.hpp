//
// Created by jazema on 4/24/26.
//

#ifndef EXPORTRENDEREXCEPTION_HPP
#define EXPORTRENDEREXCEPTION_HPP

#include <exception>
#include <string>

namespace rc
{
    class ExportRenderException: public std::exception
    {
        private:
            std::string _message;
        public:
            ExportRenderException(const std::string &fileName, const std::string &reason);
            const char *what() const noexcept override;
    };
}

#endif
