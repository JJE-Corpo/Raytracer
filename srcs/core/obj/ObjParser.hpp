
#ifndef OBJPARSER_HPP
#define OBJPARSER_HPP

#include <exception>

#include "../../common/scene/IScene.hpp"
#include "../../common/Vector.hpp"

namespace rc
{
    class ObjParser
    {
        private:
            std::vector<ISceneObject *> &_objects;
            
            std::string _content;
            Vector3f _position = Vector3f(0.0f, 0.0f, 0.0f);
            Vector3f _rotation = Vector3f(0.0f, 0.0f, 0.0f);
            float _size = 1.0f;

            std::vector<Vector3f> _vertices;
            std::vector<Vector3f> _normals;

            std::size_t _triangleNumber = 0;

            void openFile(const std::string &file_path);

            void parseVertex(const std::string &line);
            void parseNormal(const std::string &line);
            void parseFace(const std::string &line, std::size_t line_number);

        public:
            ObjParser(std::vector<ISceneObject *> &objects);
            ~ObjParser();

            void parse(const std::string &file_path);

            void withPosition(const Vector3f &position);
            void withRotation(const Vector3f &rotation);
            void withSize(float size);
    };

    class ObjParserException : public std::exception
    {
        private:
            std::string _message;

        public:
            ObjParserException(const std::string &message) : _message(message) {}
            virtual const char* what() const noexcept override
            {
                return this->_message.c_str();
            }
    };
}

#endif