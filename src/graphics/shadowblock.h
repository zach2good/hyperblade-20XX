#ifndef HYPERBLADE_SHADOWBLOCK_H
#define HYPERBLADE_SHADOWBLOCK_H

#include "graphics/meshblock.h"
#include "graphics/recordoffset.h"
#include "math/vector.h"

// The shadow mesh a model carries.
struct ShadowBlock
{
    short              nFaceCount;   // 0x00
    short              nVertexCount; // 0x02
    RecordOffset<Vec3> vertexOffset; // 0x04
    // One component link id a vertex.
    RecordOffset<short> indexOffset; // 0x06
    ShadowFace          aFaces[1];   // 0x08, nFaceCount long

    Vec3* Vertices()
    {
        return vertexOffset.In(this);
    }

    short* Indices()
    {
        return indexOffset.In(this);
    }
};

#endif
