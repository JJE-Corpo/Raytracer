#ifndef SCENEOBJECTBUILDER_HPP
#define SCENEOBJECTBUILDER_HPP

#include "LightBuilder.hpp"
#include "PrimitiveBuilder.hpp"

#define LIGHT_POINT "point_light"
#define LIGHT_DIRECTIONAL "directional_light"

#define PRIMITIVE_SPHERE "sphere"
#define PRIMITIVE_CUBE "cube"
#define PRIMITIVE_CYLINDER "cylinder"
#define PRIMITIVE_CONE "cone"
#define PRIMITIVE_PLANE "plane"
#define PRIMITIVE_TRIANGLE "triangle"
#define PRIMITIVE_TANGLECUBE "tanglecube"
#define PRIMITIVE_FRACTAL "fractal"
#define PRIMITIVE_TORUS "torus"

namespace rc
{
    class SceneObjectBuilder
    {
        private:
            ObjectType _objectType;
            bool _hasObjectType;
            PrimitiveBuilder _primitiveBuilder;
            LightBuilder _lightBuilder;

        public:
            SceneObjectBuilder();
            ~SceneObjectBuilder() = default;

            SceneObjectBuilder &withName(const std::string &name);
            SceneObjectBuilder &withType(const std::string &type);
            SceneObjectBuilder &withType(LightType type);
            SceneObjectBuilder &withPosition(Vector3f position);
            SceneObjectBuilder &withRotation(Vector3f rotation);
            SceneObjectBuilder &withScale(Vector3f scale);
            SceneObjectBuilder &withVertex0(Vector3f vertex0);
            SceneObjectBuilder &withVertex1(Vector3f vertex1);
            SceneObjectBuilder &withVertex2(Vector3f vertex2);
            SceneObjectBuilder &withRadius(float radius);
            SceneObjectBuilder &withHeight(float height);
            SceneObjectBuilder &withAxis(const Axis &axis);
            SceneObjectBuilder &withSize(float size);
            SceneObjectBuilder &withMaterial(const Material *material);
            SceneObjectBuilder &withPower(float power);
            SceneObjectBuilder &withIterations(int iterations);
            SceneObjectBuilder &withThreshold(float threshold);
            SceneObjectBuilder &withDirection(Vector3f direction);
            SceneObjectBuilder &withIntensity(float intensity);
            SceneObjectBuilder &withColor(Color color);

            ISceneObject *build() const;
    };
}

#endif