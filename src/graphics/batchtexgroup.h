// One run of triangles sharing a texture in the D3D face batch.

#ifndef HYPERBLADE_BATCHTEXGROUP_H
#define HYPERBLADE_BATCHTEXGROUP_H

struct BatchTexGroup
{
    unsigned int   dwTextureHandle; // 0x00
    unsigned short wTriangleCount;  // 0x04
    unsigned short wUnused;         // 0x06
};

static_assert(sizeof(BatchTexGroup) == 8);

#endif
