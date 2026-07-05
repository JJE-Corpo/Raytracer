#ifndef PRIMITIVEBUILDER_HPP
#define PRIMITIVEBUILDER_HPP

#include "../../../common/Axis.hpp"
#include "../../../common/Color.hpp"
#include "../../../common/scene/IPrimitive.hpp"
#include "../../../common/Material.hpp"
#include "../../../common/Vector.hpp"

namespace rc
{
    class PrimitiveBuilder
    {
        private:
            std::string _name;
            std::string _type;

            Vector3f _position = Vector3f(0.0f, 0.0f, 0.0f);
            Vector3f _rotation = Vector3f(0.0f, 0.0f, 0.0f);
            Vector3f _scale = Vector3f(1.0f, 1.0f, 1.0f);
            Vector3f _center;
            Vector3f _vertex0;
            Vector3f _vertex1;
            Vector3f _vertex2;

            float _radius = 100.0f;
            float _height = 100.0f;
            float _size = 100.0f;
            float _threshold = 0.0f;
            float _power = 8.0f;
            int _iterations = 8;
            Axis _axis = Axis::Z;
            std::string _file;
            const Material *_material = nullptr;
        public:
            PrimitiveBuilder() = default;
            ~PrimitiveBuilder() = default;

            PrimitiveBuilder &withName(const std::string &name);
            PrimitiveBuilder &withType(const std::string &type);
            PrimitiveBuilder &withPosition(Vector3f position);
            PrimitiveBuilder &withRotation(Vector3f rotation);
            PrimitiveBuilder &withScale(Vector3f scale);
            PrimitiveBuilder &withVertex0(Vector3f vertex0);
            PrimitiveBuilder &withVertex1(Vector3f vertex1);
            PrimitiveBuilder &withVertex2(Vector3f vertex2);
            PrimitiveBuilder &withRadius(float radius);
            PrimitiveBuilder &withHeight(float height);
            PrimitiveBuilder &withAxis(const Axis &axis);
            PrimitiveBuilder &withSize(float size);
            PrimitiveBuilder &withFile(const std::string &file);
            PrimitiveBuilder &withMaterial(const Material *material);
            PrimitiveBuilder &withPower(float power);
            PrimitiveBuilder &withIterations(int iterations);
            PrimitiveBuilder &withThreshold(float threshold);

            IPrimitive *build() const;
    };
}

#endif
