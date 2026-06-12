//
// Created by jazema on 5/1/26.
//

#ifndef VERTICALLAYOUT_HPP
#define VERTICALLAYOUT_HPP

namespace rc
{
    struct VerticalLayout
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
