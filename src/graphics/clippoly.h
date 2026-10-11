#ifndef HYPERBLADE_GRAPHICS_CLIPPOLY_H
#define HYPERBLADE_GRAPHICS_CLIPPOLY_H

#include "common/staticassert.h"
#include "graphics/texcoord.h"
#include "graphics/transformedvertex.h"
#include "math/vector.h"
#include <stddef.h>

// Bit n of a buffer's mask names component n: the point's four, three no renderer sets, then the UV pair.
struct ClipVertex
{
    union
    {
        float af[9];

        struct
        {
            TransformedVertex pos;
            Vec3              vUnused;
            TexCoord          uv;
        };
    };
};

// The polygon the clipper reads and writes. The buffers below give it room for its vertices.
struct ClipPolyBuffer
{
    int        nCount;
    int        nMask;
    int        nUnused;
    ClipVertex aVertices[1]; // nCount long
};

// A renderer's face. Twenty four vertices is what fits before the next global.
struct FacePolyBuffer : ClipPolyBuffer
{
    ClipVertex aMoreVertices[23];
};

// The clipper's scratch polygon, which a clip can grow past a face's twenty four.
struct ClipStagingBuffer : ClipPolyBuffer
{
    ClipVertex aMoreVertices[31];
};

STATIC_ASSERT(sizeof(ClipVertex) == 0x24);
STATIC_ASSERT(offsetof(ClipPolyBuffer, aVertices) == 0xc);
STATIC_ASSERT(sizeof(FacePolyBuffer) == 0x36c);
STATIC_ASSERT(sizeof(ClipStagingBuffer) == 0x48c);

#endif
