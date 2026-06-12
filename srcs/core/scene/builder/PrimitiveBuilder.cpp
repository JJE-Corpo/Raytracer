#include "PrimitiveBuilder.hpp"

#include "SceneObjectBuilder.hpp"

#include "../../../plugins/primitive/Fractal.hpp"
#include "../../../plugins/primitive/Cylinder.hpp"
#include "../../../plugins/primitive/Plane.hpp"
#include "../../../plugins/primitive/Triangle.hpp"
#include "../../../plugins/primitive/Sphere.hpp"
#include "../../../plugins/primitive/Tanglecube.hpp"
#include "../../../plugins/primitive/Cube.hpp"
#include "../../../plugins/primitive/Cone.hpp"
#include "../../../plugins/primitive/Torus.hpp"

rc::PrimitiveBuilder &rc::PrimitiveBuilder::withName(const std::string &name)
{
    this->_name = name;
    return (*this);
}

rc::PrimitiveBuilder &rc::PrimitiveBuilder::withType(const std::string &type)
{
    this->_type = type;
    return (*this);
}


rc::PrimitiveBuilder &rc::PrimitiveBuilder::withPosition(const Vector3f position)
{
    this->_position = position;
    return (*this);
}

rc::PrimitiveBuilder &rc::PrimitiveBuilder::withRotation(const Vector3f rotation)
{
    this->_rotation = rotation;
    return (*this);
}

rc::PrimitiveBuilder &rc::PrimitiveBuilder::withScale(Vector3f scale)
{
    this->_scale = scale;
    return (*this);
}

rc::PrimitiveBuilder &rc::PrimitiveBuilder::withVertex0(Vector3f vertex0)
{
    this->_vertex0 = vertex0;
    return (*this);
}


rc::PrimitiveBuilder &rc::PrimitiveBuilder::withVertex1(Vector3f vertex1)
{
    this->_vertex1 = vertex1;
    return (*this);
}


rc::PrimitiveBuilder &rc::PrimitiveBuilder::withVertex2(Vector3f vertex2)
{
    this->_vertex2 = vertex2;
    return (*this);
}


rc::PrimitiveBuilder &rc::PrimitiveBuilder::withRadius(const float radius)
{
    this->_radius = radius;
    return (*this);
}


rc::PrimitiveBuilder &rc::PrimitiveBuilder::withHeight(const float height)
{
    this->_height = height;
    return (*this);
}

rc::PrimitiveBuilder &rc::PrimitiveBuilder::withAxis(const Axis &axis)
{
    this->_axis = axis;
    return (*this);
}

rc::PrimitiveBuilder &rc::PrimitiveBuilder::withSize(float size)
{
    this->_size = size;
    return (*this);
}

rc::PrimitiveBuilder &rc::PrimitiveBuilder::withPower(float power)
{
    this->_power = power;
    return (*this);
}

rc::PrimitiveBuilder &rc::PrimitiveBuilder::withIterations(int iterations)
{
    this->_iterations = iterations;
    return (*this);
}

rc::PrimitiveBuilder &rc::PrimitiveBuilder::withMaterial(const Material *material)
{
    this->_material = material;
    return (*this);
}

rc::PrimitiveBuilder &rc::PrimitiveBuilder::withThreshold(float threshold)
{
    this->_threshold = threshold;
    return (*this);
}

rc::IPrimitive *rc::PrimitiveBuilder::build() const
{
    if (this->_type == PRIMITIVE_SPHERE)
        return (new Sphere(this->_name, this->_position, this->_scale, this->_radius, this->_material));
    if (this->_type == PRIMITIVE_CYLINDER)
        return (new Cylinder(this->_name, this->_position, this->_rotation, this->_scale, this->_radius, this->_height, this->_material));
    if (this->_type == PRIMITIVE_PLANE)
        return (new Plane(this->_name, this->_position, AxisUtils::toVector(this->_axis), this->_size, this->_material));
    if (this->_type == PRIMITIVE_TRIANGLE)
        return (new Triangle(this->_name, this->_vertex0, this->_vertex1, this->_vertex2, this->_material));
    if (this->_type == PRIMITIVE_TANGLECUBE)
        return (new Tanglecube(this->_name, this->_position, this->_rotation, this->_threshold, this->_size, this->_material));
    if (this->_type == PRIMITIVE_FRACTAL)
        return (new Fractal(this->_name, this->_center, this->_size, this->_power, this->_iterations, this->_material));
    if (this->_type == PRIMITIVE_CUBE)
        return (new Cube(this->_name, this->_position, this->_rotation, this->_scale, this->_size, this->_material));
    if (this->_type == PRIMITIVE_CONE)
        return (new Cone(this->_name, this->_position, this->_rotation, this->_scale, static_cast<float>(this->_radius), this->_height, this->_material));
    if (this->_type == PRIMITIVE_TORUS)
        return (new Torus(this->_name, this->_position, this->_rotation, static_cast<float>(this->_radius), this->_height, this->_material));
    return (nullptr);
}
