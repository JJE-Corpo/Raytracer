//
// Created by jazema on 5/1/26.
//
// A small cursor that walks down the Y axis as widgets are placed one under
// the other inside a panel's layout() method - not to be confused with Screen
// (screens/Screen.hpp), which is the top-level "what fills the window" concept.
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
