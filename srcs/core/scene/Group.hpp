#ifndef GROUP_HPP
#define GROUP_HPP

#include <string>

#include "../../common/scene/ASceneObject.hpp"

namespace rc
{
    class Group : public ASceneObject
    {
        public:
            Group() { this->_name = "Group"; }
            explicit Group(const std::string &name) { this->_name = name; }
            ~Group() override = default;

            ObjectType getObjectType() const override { return ObjectType::GROUP; }
            std::string getTypeName() const override { return "group"; }
    };
}

#endif
