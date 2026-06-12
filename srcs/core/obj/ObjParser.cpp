#include "ObjParser.hpp"

#include "../scene/builder/SceneObjectBuilder.hpp"
#include "../../common/Matrix.hpp"

#include <fstream>
#include <sstream>
#include <iostream>

rc::ObjParser::ObjParser(std::vector<ISceneObject*> &objects) : _objects(objects)
{
}

rc::ObjParser::~ObjParser()
{
}

void rc::ObjParser::parse(const std::string &file_path)
{
    this->openFile(file_path);

    std::istringstream file_stream(this->_content);
    std::string line;
    std::size_t line_number = 0;

    while (std::getline(file_stream, line))
    {
        line_number++;

        if (line.empty() || line[0] == '#')
            continue;

        if (line.substr(0, 2) == "v ")
            this->parseVertex(line);
        else if (line.substr(0, 3) == "vn ")
            this->parseNormal(line);
        else if (line.substr(0, 2) == "f ")
            this->parseFace(line, line_number);
    }
}

void rc::ObjParser::openFile(const std::string &file_path)
{
    std::ifstream file(file_path);
    if (!file.is_open())
    {
        throw ObjParserException("Could not open file: " + file_path);
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    this->_content = buffer.str();
}

void rc::ObjParser::parseVertex(const std::string &line)
{
    std::istringstream line_stream(line.substr(2));
    float x, y, z;
    
    line_stream >> x >> y >> z;
    this->_vertices.emplace_back(x, y, z);
}

void rc::ObjParser::parseNormal(const std::string &line)
{
    std::istringstream line_stream(line.substr(3));
    float x, y, z;
    
    line_stream >> x >> y >> z;
    this->_normals.emplace_back(x, y, z);
}

void rc::ObjParser::parseFace(const std::string &line, std::size_t line_number)
{
    std::istringstream line_stream(line.substr(2));
    std::string vertex_info;
    std::vector<std::tuple<int, int, int>> vertices;

    while (line_stream >> vertex_info)
    {
        std::istringstream vertex_stream(vertex_info);
        std::string index_str;
        int vertex_index = 0;
        int normal_index = 0;

        std::getline(vertex_stream, index_str, '/');
        vertex_index = std::stoi(index_str);

        if (vertex_stream.peek() == '/')
        {
            vertex_stream.get();
            if (vertex_stream.peek() != '/')
            {
                std::getline(vertex_stream, index_str, '/');
                // We ignore texture coordinates for now
            }
        }

        if (vertex_stream.peek() == '/')
        {
            vertex_stream.get();
            std::getline(vertex_stream, index_str);
            normal_index = std::stoi(index_str);
        }

        vertices.emplace_back(vertex_index - 1, 0, normal_index - 1);
    }

    // For simplicity, we only handle triangular faces
    if (vertices.size() != 3)
    {
        throw ObjParserException("Only triangular faces are supported");
    }

    // std::cout << "Parsed face with vertices: " << std::get<0>(vertices[0]) << ", " << std::get<0>(vertices[1]) << ", " << std::get<0>(vertices[2]) << std::endl;
    // std::cout << "Parsed face with normals: " << std::get<2>(vertices[0]) << ", " << std::get<2>(vertices[1]) << ", " << std::get<2>(vertices[2]) << std::endl;

    Vector3f v0 = this->_vertices[std::get<0>(vertices[0])];
    Vector3f v1 = this->_vertices[std::get<0>(vertices[1])];
    Vector3f v2 = this->_vertices[std::get<0>(vertices[2])];

    // Vector3f n0 = this->_normals[std::get<2>(vertices[0])];
    // Vector3f n1 = this->_normals[std::get<2>(vertices[1])];
    // Vector3f n2 = this->_normals[std::get<2>(vertices[2])];

    this->_triangleNumber++;

    // Apply transformations: scale, rotate, then translate
    v0 = v0 * this->_size;
    v1 = v1 * this->_size;
    v2 = v2 * this->_size;

    Matrix<4> rotation = Matrix<4>::rotation_x(this->_rotation.x)
                       * Matrix<4>::rotation_y(this->_rotation.y)
                       * Matrix<4>::rotation_z(this->_rotation.z);
    
    v0 = rotation * v0;
    v1 = rotation * v1;
    v2 = rotation * v2;

    v0 = v0 + this->_position;
    v1 = v1 + this->_position;
    v2 = v2 + this->_position;

    // Create a triangle primitive and add it to the scene
    SceneObjectBuilder builder;
    
    builder.withType(PRIMITIVE_TRIANGLE)
           .withName("Triangle #" + std::to_string(this->_triangleNumber))
           .withVertex0(v0)
           .withVertex1(v1)
           .withVertex2(v2);

    ISceneObject *object = builder.build();
    if (!object)
        throw ObjParserException("[Line " + std::to_string(line_number) + "] Failed to create primitive for face");

    this->_objects.push_back(object);
}

void rc::ObjParser::withPosition(const Vector3f &position)
{
    this->_position = position;
}

void rc::ObjParser::withRotation(const Vector3f &rotation)
{
    this->_rotation = rotation;
}

void rc::ObjParser::withSize(float size)
{
    this->_size = size;
}
