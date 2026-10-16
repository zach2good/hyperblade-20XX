#ifndef HYPERBLADE_GRAPHICS_TRANSFORMEDVERTEX_H
#define HYPERBLADE_GRAPHICS_TRANSFORMEDVERTEX_H

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

static_assert(sizeof(TransformedVertex) == 0x10);

#endif
