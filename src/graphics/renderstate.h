// Thirteen ints and two floats from 0x004cbb88, then the Direct3D state block at 0x004cbbc8.

#ifndef HYPERBLADE_RENDERSTATE_H
#define HYPERBLADE_RENDERSTATE_H

#include "common/compat.h"
#include <d3d.h>
#include <ddraw.h>
#include "d3ddeviceinit.h"
#include "d3dshapes.h"
#include "common/staticassert.h"
#include <stddef.h>
#include "graphics/d3drend.h"

struct D3DBatchVertex;
struct BatchTexGroup;

// D3DRend::InitRenderState clears all 0x631 words with one rep stosd.
struct D3DStateBlock
{
    IDirectDraw* pDDraw;
    IDirect3D*   pDirect3D;
    // Clear when a secondary DirectDraw device with 3D hardware is in use.
    int bIsPrimary;
    int nD3DDriverCount;
    // Written only on success, so a failed selection leaves the previous choice.
    int nD3DDriverIndex;
    int anUnusedA[0x3f];
    // The enumeration stops at five drivers, which is what fits before the texture flag.
    D3DDriverInfo aD3DDrivers[5];
    int           bD3DTexturesEnabled;
    // The chosen texture format's index, minus one when there is none.
    int nTextureFormat;
    // Counts the formats seen, and is the slot the next one is written to.
    int nTextureFormatsSeen;
    // Ten fit before the device pointer, and the callback does not check.
    DDSURFACEDESC      aTextureFormats[10];
    IDirect3DDevice*   pDevice;
    int                nViewportCount;
    IDirect3DViewport* apViewports[2];
    // One per viewport, the rectangle D3DViewport::Clear clears.
    D3DRECT                 aViewportRects[2];
    IDirect3DExecuteBuffer* pExecuteBuffer;
    // Five dwords one past the buffer pointer, so CreateExecuteBuffer gets adjacent addresses.
    D3DEXECUTEBUFFERDESC execBufferDesc;
    unsigned int         nExecuteBufferSize;
    // Twelve dwords, cleared and refilled before every SetExecuteData.
    D3DEXECUTEDATA d3dExecuteData;
    int            nDesktopWidth;
    int            nDesktopHeight;
    int            nDesktopBpp;
    int            anUnusedB[2];
    // g_bEmulationOnly, copied when the state is set up.
    int bOnlyEmulation;
    // Deleted on release but never cleared, so a second call deletes them again.
    D3DMATRIXHANDLE    dwMatrixHandle0;
    D3DMATRIXHANDLE    dwMatrixHandle1;
    D3DMATRIXHANDLE    dwMatrixHandle2;
    IDirect3DMaterial* pD3DMaterial;
    // Only the viewport setter reads it, and nothing writes it.
    D3DMATERIALHANDLE hBackgroundMaterial;
    int               anUnusedC[7];
    D3DBatchVertex*   pBatchVertexBase;
    D3DBatchVertex*   pBatchVertex;
    int               nBatchVertexCount;
    BatchTexGroup*    pBatchTexGroupBase;
    BatchTexGroup*    pBatchTexGroup;
    int               nBatchTexHandle;
    D3DTRIANGLE*      pBatchTriangleBase;
    D3DTRIANGLE*      pBatchTriangle;
    int               nBatchTriangleCount;
    int               nBatchInstructionOffset;
};

STATIC_ASSERT(offsetof(D3DStateBlock, pDirect3D) == 0x4);
STATIC_ASSERT(offsetof(D3DStateBlock, nD3DDriverIndex) == 0x10);
STATIC_ASSERT(offsetof(D3DStateBlock, aD3DDrivers) == 0x110);
STATIC_ASSERT(offsetof(D3DStateBlock, bD3DTexturesEnabled) == 0x1394);
STATIC_ASSERT(offsetof(D3DStateBlock, aTextureFormats) == 0x13a0);
STATIC_ASSERT(offsetof(D3DStateBlock, pDevice) == 0x17d8);
STATIC_ASSERT(offsetof(D3DStateBlock, aViewportRects) == 0x17e8);
STATIC_ASSERT(offsetof(D3DStateBlock, execBufferDesc) == 0x180c);
STATIC_ASSERT(offsetof(D3DStateBlock, d3dExecuteData) == 0x1824);
STATIC_ASSERT(offsetof(D3DStateBlock, nDesktopWidth) == 0x1854);
STATIC_ASSERT(offsetof(D3DStateBlock, bOnlyEmulation) == 0x1868);
STATIC_ASSERT(offsetof(D3DStateBlock, hBackgroundMaterial) == 0x187c);
STATIC_ASSERT(offsetof(D3DStateBlock, pBatchVertexBase) == 0x189c);
STATIC_ASSERT(offsetof(D3DStateBlock, nBatchInstructionOffset) == 0x18c0);
STATIC_ASSERT(sizeof(D3DStateBlock) == 0x631 * 4);

// GLOBAL: HYPERX 0x004cbbc8
extern D3DStateBlock g_d3dState;

#endif
