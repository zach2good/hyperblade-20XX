// A face record of a mesh block, and the shadow face that runs its corner array on to four.

#ifndef HYPERBLADE_MESHFACE_H
#define HYPERBLADE_MESHFACE_H

#include "graphics/face.h"
#include "math/vector.h"

#pragma pack(push, 1)

struct MeshFace
{
    short         nCorners;  // 0x00
    unsigned char cFlags;    // 0x02
    char          cMaterial; // 0x03
    Vec3          vNormal;   // 0x04
    // Corners, a second corner block when flag 0x10 is set, a UV index per corner, then a width.
    short aCorners[1]; // 0x10

    short* UVIndices();

    // The texture width less one the UVs were last scaled to, or minus one with no UVs.
    short GetScaledWidth();
    void  SetScaledWidth(short nWidth);
};

// A shadow face: a MeshFace whose corner array runs on to four.
struct ShadowFace
{
    MeshFace face;            // 0x00
    short    aMoreCorners[3]; // 0x12
};

#pragma pack(pop)

inline short* MeshFace::UVIndices()
{
    int nSkip;

    if ((cFlags & kFaceFlagUVSkip) != 0)
    {
        nSkip = nCorners;
    }
    else
    {
        nSkip = 0;
    }

    return &aCorners[nSkip + nCorners];
}

inline short MeshFace::GetScaledWidth()
{
    if ((cFlags & kFaceFlagHasUV) != 0)
    {
        return UVIndices()[nCorners];
    }

    return -1;
}

inline void MeshFace::SetScaledWidth(short nWidth)
{
    if ((cFlags & kFaceFlagHasUV) != 0)
    {
        UVIndices()[nCorners] = nWidth;
    }
}

static_assert(sizeof(ShadowFace) == 0x18);
#endif
