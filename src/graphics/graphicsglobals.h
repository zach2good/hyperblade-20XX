// Globals only the graphics files read. Their definitions, in .data order, are in src/globals.cpp.

#ifndef HYPERBLADE_GRAPHICSGLOBALS_H
#define HYPERBLADE_GRAPHICSGLOBALS_H
#include "common/compat.h"

#include "globals.h"
#include "graphics/surfacepoint.h"
#include "renderoptions.h"
#include <stddef.h>

// A lookup matches on the resource name.
struct BitmapCacheEntry
{
    HBITMAP     hBitmap;
    const char* pszName;
};

// Gates the lighting info display together with g_bNetworkGame. Nothing else reads it.
// GLOBAL: HYPERX 0x004d2b7d
extern unsigned char g_bShowLightingInfo;

// GLOBAL: HYPERX 0x004ceeb8
extern int g_bBackBufferReady;

// The game window's GWL_ID word, which nothing reads.
// GLOBAL: HYPERX 0x004ceec0
extern long g_nDisplayWindowID;

// GLOBAL: HYPERX 0x004ceea0
extern HDC g_hScreenBltDC;

// GLOBAL: HYPERX 0x004be6b0
extern unsigned int g_dwDisplayWidth;

// GLOBAL: HYPERX 0x004be6b4
extern unsigned int g_dwDisplayHeight;

// GLOBAL: HYPERX 0x004cef50
extern unsigned int g_dwFrameGameLogicStart;

struct SoftTexImage;
struct ClipVertex;
struct DrawPolySurface;

// The texture DLL's two entry points, loaded by name.
typedef void (*MdmDrawTexPolyFn)(SoftTexImage* pTexture, ClipVertex* pVertices, int nCount, int nEdges);
typedef void (*MdmInitDrawPolyFn)(DrawPolySurface* pSurface, int nOriginX, int nOriginY);

// GLOBAL: HYPERX 0x004ced58
extern MdmDrawTexPolyFn g_pfnMdmDrawTexPoly;

// GLOBAL: HYPERX 0x004ced60
extern MdmInitDrawPolyFn g_pfnMdmInitDrawPoly;

// GLOBAL: HYPERX 0x004b3e20
extern const float g_flLightAzimuth180;

// GLOBAL: HYPERX 0x004b3e24
extern const float g_flLightAzimuth54;

// GLOBAL: HYPERX 0x004b3e28
extern const float g_flLightAzimuth303;

// GLOBAL: HYPERX 0x004d0a00
extern Vec3 g_vLightDirExtra;

// The head of the software texture list, threaded through each texture's m_pNext.
// GLOBAL: HYPERX 0x004d2b78
extern class SoftTex* g_pSoftTexHead;

// GLOBAL: HYPERX 0x004b38e4
extern const float g_flZeroMeshBone;

// GLOBAL: HYPERX 0x004d21b0
extern int g_nBitmapCacheCount;

// GLOBAL: HYPERX 0x004d21b4
extern BitmapCacheEntry* g_pBitmapCache;

// GLOBAL: HYPERX 0x004ce9b8
extern unsigned char g_bEdgeWalked;

// GLOBAL: HYPERX 0x004ce9bc
extern int g_nFillColour;

// GLOBAL: HYPERX 0x004ced3c
extern unsigned char g_bRightEdgeUsed;

// GLOBAL: HYPERX 0x004ced3d
extern unsigned char g_bLeftEdgeUsed;

// GLOBAL: HYPERX 0x004ced3e
extern short g_wSpanYMax;

// The minimum is the topmost row; the fill resets it to the last row first.
// GLOBAL: HYPERX 0x004ced40
extern short g_wSpanYMin;

struct SoftBackBitmapInfo
{
    BITMAPINFOHEADER bmiHeader;
    RGBQUAD          bmiColors[256];
};

static_assert(offsetof(SoftBackBitmapInfo, bmiColors) == 0x28);
static_assert(sizeof(SoftBackBitmapInfo) == 0x428);

// GLOBAL: HYPERX 0x004bc500
extern SoftBackBitmapInfo g_bmiSoftBackBuffer;

// The word after it points at it; nothing known reads either.
// GLOBAL: HYPERX 0x004c2cb4
extern char g_szTopDownDibName[12];

// GLOBAL: HYPERX 0x004c2cc0
extern char* g_pszDibOrientationName;

struct LayoutState;
extern LayoutState g_cameraViewportBlock;

// The three render option words. Renderer::GetRenderOptions copies all three as one structure.
// GLOBAL: HYPERX 0x004cee50
extern RenderOptions g_renderOptions;

// The visible faces DrawFaces records and the list it sorts and draws them through.
// GLOBAL: HYPERX 0x004d0650
extern struct FaceRecord** g_ppFaceList;
// GLOBAL: HYPERX 0x004d0654
extern struct FaceRecord* g_pFaceRecords;

#endif
