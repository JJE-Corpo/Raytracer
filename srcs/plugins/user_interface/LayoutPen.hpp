//
// Created by jazema on 5/1/26.
//

#ifndef LAYOUTPEN_HPP
#define LAYOUTPEN_HPP

namespace rc
{
    struct LayoutPen
    {
        float x;
        float y;
        float spacing = 10.f;

        void next(float h)
        {
            y += h + spacing;
        }
    };
}

#endif
