#ifndef HYPERBLADE_GRAPHICS_TRANSFORMEDVERTEX_H
#define HYPERBLADE_GRAPHICS_TRANSFORMEDVERTEX_H

#include "common/staticassert.h"

// Every reader divides by the fourth word, a w.
struct TransformedVertex
{
    union
    {
        float af[4];

        struct
        {
            float x;
            float y;
            float z;
            float w;
        };
    };
};

STATIC_ASSERT(sizeof(TransformedVertex) == 0x10);

#endif
