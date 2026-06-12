//
// Created by jazema on 4/21/26.
//

#ifndef ISCENERENDERER_HPP
#define ISCENERENDERER_HPP

#include "Color.hpp"
#include "IPlugin.hpp"
#include "scene/IScene.hpp"
#include <string>

namespace rc
{
    struct Render
    {
        int size_x;
        int size_y;
        std::vector<Color> pixels;
    };
    template<int W, int H>
    struct TileBuffer
    {
        Color pixels[W * H];
    };
    class ISceneRenderer : public IPlugin
    {
        public:
            virtual ~ISceneRenderer() = default;

            virtual void renderScene(const IScene &scene) = 0;
            virtual void setPixel(int x, int y, Color color) = 0;

            virtual void stopRendering() = 0;

            virtual bool isRendering() const = 0;

            virtual int getCurrentSample() const = 0;

            virtual Render getRender() const = 0;

            virtual std::string getRendererName() const = 0;

            PluginType getType() const override = 0;
    };
}

#endif
