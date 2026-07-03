/*
** EPITECH PROJECT, 2025
** raytracer [WSL: Ubuntu-22.04]
** File description:
** Fractal.hpp
*/

#ifndef RAYTRACER_FRACTAL_HPP
    #define RAYTRACER_FRACTAL_HPP

    #include "../../common/Color.hpp"
    #include "../../common/scene/IPrimitive.hpp"
    #include "../../common/Material.hpp"
    #include "../../common/Vector.hpp"
    #include "../../common/AABB.hpp"
    #include <string>

namespace rc
{

    enum class FractalType {
        MANDELBULB,
        MANDELBOX,
        JULIA3D,
        MENGER_SPONGE,
        SIERPINSKI
    };

    class Fractal : public IPrimitive
    {
        private:
            std::string _name = "Fractal";
            FractalType _type = FractalType::MANDELBULB;
            Vector3f _juliaC = {0.355f, 0.355f, 0.355f};
            float _mandelboxScale = 2.0f;
            float _mandelboxMinRadius = 0.5f;
            float _mandelboxFixedRadius = 1.0f;
            float _foldingLimit = 1.0f;
            float _sierpinskiScale = 2.0f;
            Vector3f _center = {0.0f, 0.0f, 0.0f};
            float _size = 1.0f;
            float _power = 8.0f;
            int _iterations = 8;
            float _bailout = 2.0f;
            float _epsilon = 0.001f;
            int _maxRaySteps = 128;
            const Material *_material;
            Vector3f _rotation = {0.0f, 0.0f, 0.0f};
            Vector3f _scale = {1.0f, 1.0f, 1.0f};
            bool _hidden = false;
            AABB _bbox = {Vector3f(-1.5f, -1.5f, -1.5f), Vector3f(1.5f, 1.5f, 1.5f)};

            float distanceEstimator(const Vector3f &pos) const;

            float deMandelbulb(const Vector3f &pos) const;
            float deMandelbox(const Vector3f &pos) const;
            float deJulia(const Vector3f &pos) const;
            float deMengerSponge(const Vector3f &pos) const;
            float deSierpinski(const Vector3f &pos) const;

        public:
            Fractal() = default;
            Fractal(const std::string& name, const Vector3f &center, float size, float power, int iterations, const Material *material, FractalType type = FractalType::MANDELBULB);
            bool intersect(const Ray &ray, float tMin, float tMax, Intersection &hit) const override;
            bool isFinite() const override;
            AABB bounding_box() const override;

            std::string getName() const override;
            void setName(const std::string &name) override { this->_name = name; }
            std::string getTypeName() const override;

            std::map<std::string, std::pair<std::string, PropertyType>> getProperties() const override;
            void setPropertyFloat(const std::string &key, float value) override;

            Vector3f getPosition() const override;
            Vector3f getRotation() const override;
            Vector3f getScale() const override;
            void setPosition(const Vector3f &position) override;
            void setRotation(const Vector3f &rotation) override;
            void setScale(const Vector3f &scale) override;

            const Material *getMaterial() const override;
            void setMaterial(const Material *material) override;

            bool isHidden() const override;
            void setHidden(bool hidden) override;
    };
}

#endif