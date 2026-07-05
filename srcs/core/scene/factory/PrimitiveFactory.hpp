
#ifndef PRIMITIVEFACTORY_HPP
#define PRIMITIVEFACTORY_HPP

#include "../../../common/scene/IPrimitive.hpp"

#include "../builder/SceneObjectBuilder.hpp"

#include "../../../plugins/primitive/Sphere.hpp"
#include "../../../plugins/primitive/Plane.hpp"
#include "../../../plugins/primitive/Cube.hpp"
#include "../../../plugins/primitive/Tanglecube.hpp"
#include "../../../plugins/primitive/Torus.hpp"
#include "../../../plugins/primitive/Triangle.hpp"
#include "../../../plugins/primitive/Fractal.hpp"
#include "../../../plugins/primitive/Cone.hpp"
#include "../../../plugins/primitive/Cylinder.hpp"
#include "../../../plugins/primitive/mesh/Mesh.hpp"

namespace rc
{
    class PrimitiveFactory
    {   
        public:
            static IPrimitive *createPrimitive(std::string type)
            {
                if (type == PRIMITIVE_PLANE)
                {
                    return new Plane();
                }
                else if (type == PRIMITIVE_SPHERE)
                {
                    return new Sphere();
                }
                else if (type == PRIMITIVE_CUBE)
                {
                    return new Cube();
                }
                else if (type == PRIMITIVE_TANGLECUBE)
                {
                    return new Tanglecube();
                }
                else if (type == PRIMITIVE_TORUS)
                {
                    return new Torus();
                }
                else if (type == PRIMITIVE_TRIANGLE)
                {
                    return new Triangle();
                }
                else if (type == PRIMITIVE_FRACTAL)
                {
                    return new Fractal();
                }
                else if (type == PRIMITIVE_CONE)
                {
                    return new Cone();
                }
                else if (type == PRIMITIVE_CYLINDER)
                {
                    return new Cylinder();
                }
                else if (type == PRIMITIVE_MESH)
                {
                    return new Mesh();
                }
                else
                {
                    throw std::runtime_error("Unknown primitive type: " + type);
                }
            }
    };
}

#endif
