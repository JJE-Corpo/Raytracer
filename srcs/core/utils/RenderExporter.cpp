//
// Created by jazema on 4/24/26.
//

#include "RenderExporter.hpp"

#include <fstream>
#include <iostream>

#include "../exceptions/ExportRenderException.hpp"

namespace rc
{
    void RenderExporter::saveToFile(const Render &render, const std::string &export_path)
    {
        std::ofstream out;

        out.open(export_path, std::ios::trunc);
        if (!out.is_open())
            throw ExportRenderException(export_path, "Could not open file");
        out << "P3" << '\n';
        out << render.size_x << " " << render.size_y << '\n';
        out << "255" << '\n';
        for (auto &pixel : render.pixels)
        {
            out << static_cast<int>(pixel.r) << " "
                << static_cast<int>(pixel.g) << " "
                << static_cast<int>(pixel.b) << '\n';
        }
        out.close();
    }
}
