#ifndef ASCENEOBJECT_HPP
#define ASCENEOBJECT_HPP

#include <string>
#include <vector>

#include "ISceneObject.hpp"

namespace rc
{
    class ASceneObject : public virtual ISceneObject
    {
        protected:
            ISceneObject *_parent = nullptr;
            std::vector<ISceneObject *> _children;

            Vector3f _localPosition = {0.0f, 0.0f, 0.0f};
            Vector3f _localRotation = {0.0f, 0.0f, 0.0f};
            Vector3f _localScale = {1.0f, 1.0f, 1.0f};

            Vector3f _worldPosition = {0.0f, 0.0f, 0.0f};
            Vector3f _worldRotation = {0.0f, 0.0f, 0.0f};
            Vector3f _worldScale = {1.0f, 1.0f, 1.0f};

            std::string _name = "Object";
            bool _hidden = false;

        public:
            ~ASceneObject() override = default;

            ISceneObject *getParent() const override { return this->_parent; }
            void setParent(ISceneObject *parent) override { this->_parent = parent; }
            const std::vector<ISceneObject *> &getChildren() const override { return this->_children; }
            void addChild(ISceneObject *child) override;
            void insertChild(ISceneObject *child, std::size_t index) override;
            void removeChild(ISceneObject *child) override;

            Vector3f getLocalPosition() const override { return this->_parent ? this->_localPosition : this->getPosition(); }
            Vector3f getLocalRotation() const override { return this->_parent ? this->_localRotation : this->getRotation(); }
            Vector3f getLocalScale() const override { return this->_parent ? this->_localScale : this->getScale(); }
            void setLocalPosition(const Vector3f &position) override;
            void setLocalRotation(const Vector3f &rotation) override;
            void setLocalScale(const Vector3f &scale) override;

            Vector3f getPosition() const override { return this->_worldPosition; }
            Vector3f getRotation() const override { return this->_worldRotation; }
            Vector3f getScale() const override { return this->_worldScale; }
            void setPosition(const Vector3f &position) override { this->_worldPosition = position; }
            void setRotation(const Vector3f &rotation) override { this->_worldRotation = rotation; }
            void setScale(const Vector3f &scale) override { this->_worldScale = scale; }

            std::string getName() const override { return this->_name; }
            void setName(const std::string &name) override { this->_name = name; }
            bool isHidden() const override { return this->_hidden; }
            void setHidden(bool hidden) override { this->_hidden = hidden; }
    };
}

#endif
