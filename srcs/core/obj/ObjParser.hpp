
#ifndef OBJPARSER_HPP
#define OBJPARSER_HPP

#include <array>
#include <exception>

#include "../../common/scene/IScene.hpp"
#include "../../common/Vector.hpp"

namespace rc
{
    struct ObjTriangle
    {
        Vector3f v0;
        Vector3f v1;
        Vector3f v2;
        bool smooth = false;
        Vector3f n0;
        Vector3f n1;
        Vector3f n2;
    };

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

            std::vector<ObjTriangle> _rawTriangles;
            std::vector<std::array<int, 3>> _faceVertexIndices;
            bool _emitObjects = true;

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

            void setEmitObjects(bool emit);
            const std::vector<ObjTriangle> &getTriangles() const;

            const std::vector<Vector3f> &getVertices() const;
            const std::vector<std::array<int, 3>> &getFaceVertexIndices() const;
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