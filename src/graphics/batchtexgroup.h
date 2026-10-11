// One run of triangles sharing a texture in the D3D face batch.

#ifndef HYPERBLADE_BATCHTEXGROUP_H
#define HYPERBLADE_BATCHTEXGROUP_H

#include "common/staticassert.h"

struct BatchTexGroup
{
    unsigned int   dwTextureHandle; // 0x00
    unsigned short wTriangleCount;  // 0x04
    unsigned short wUnused;         // 0x06
};

STATIC_ASSERT(sizeof(BatchTexGroup) == 8);

#endif
