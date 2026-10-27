// A face record of a mesh block, and the shadow face that runs its corner array on to four.

#ifndef HYPERBLADE_MESHFACE_H
#define HYPERBLADE_MESHFACE_H

#include "common/staticassert.h"
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

// The UV indices follow the corners; a skipping face adds a second block, counted in bytes.
inline short* MeshFace::UVIndices()
{
    return (short*)((char*)aCorners + ((cFlags & FACE_FLAG_UV_SKIP) != 0 ? nCorners * sizeof(short) : 0) + nCorners * sizeof(short));
}

inline short MeshFace::GetScaledWidth()
{
    if ((cFlags & FACE_FLAG_HAS_UV) != 0)
    {
        return UVIndices()[nCorners];
    }

    return -1;
}

inline void MeshFace::SetScaledWidth(short nWidth)
{
    if ((cFlags & FACE_FLAG_HAS_UV) != 0)
    {
        UVIndices()[nCorners] = nWidth;
    }
}

STATIC_ASSERT(sizeof(ShadowFace) == 0x18);
#endif
