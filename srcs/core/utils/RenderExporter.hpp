//
// Created by jazema on 4/24/26.
//

#ifndef RENDEREXPORTER_HPP
#define RENDEREXPORTER_HPP
#include "../../common/ISceneRenderer.hpp"

namespace rc
{
    class RenderExporter
    {
        public:
            static void saveToFile(const Render &render, const std::string &export_path);
    };
}

#endif
