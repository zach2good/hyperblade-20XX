#ifndef HYPERBLADE_GRAPHICS_D3DSHAPES_H
#define HYPERBLADE_GRAPHICS_D3DSHAPES_H

#include <d3d.h>
#include "graphics/clippoly.h"
#include "graphics/texcoord.h"
#include "graphics/transformedvertex.h"

// GLOBAL: HYPERX 0x004cb810
extern FacePolyBuffer g_d3dFaceBuffer;

// GLOBAL: HYPERX 0x004cd498
extern unsigned int g_dwCurrentTexHandle;

extern class TransformObject* g_pCameraTransform;

// The texture gate reads it as a third condition.
// GLOBAL: HYPERX 0x004bd4f0
extern int g_nMdmTexturesReady;

// D3DTLVERTEX's layout, kept apart because DrawFace copies the position and the UV as records.
struct D3DBatchVertex
{
    // The screen position and reciprocal w, as the staging vertex carries them.
    TransformedVertex pos;
    unsigned int      dwColour;
    unsigned int      dwSpecular;
    TexCoord          uv;
};

static_assert(sizeof(D3DBatchVertex) == 0x20);

#endif
