// Globals only the graphics files read. Their definitions, in .data order, are in src/globals.cpp.

#ifndef HYPERBLADE_GRAPHICSGLOBALS_H
#define HYPERBLADE_GRAPHICSGLOBALS_H
#include "common/compat.h"

#include "globals.h"
#include "graphics/surfacepoint.h"
#include "renderoptions.h"
#include "graphics/ddrawcaps.h"
#include <stddef.h>

// A lookup matches on the resource name.
struct BitmapCacheEntry
{
    HBITMAP     hBitmap;
    const char* pszName;
};

struct SavedDisplayMetrics
{
    int nWidth;
    int nHeight;
    int nBpp;
    // Allocated with the record and never read or written.
    int nUnused;
};

// The result of the last DDraw::RestoreAllSurfaces call.
// GLOBAL: HYPERX 0x004cee6c
extern int g_hrRestoreResult;

// Gates the lighting info display together with g_bNetworkGame. Nothing else reads it.
// GLOBAL: HYPERX 0x004d2b7d
extern unsigned char g_bShowLightingInfo;

// The DirectDraw object and the front surface that the Texture constructor is handed.
// GLOBAL: HYPERX 0x004cd4c0
extern IDirectDraw* g_pSurfaceDDraw;

// GLOBAL: HYPERX 0x004cd4c4
extern IDirectDrawSurface* g_pSurfaceFront;

// x for surfaces in system memory, y for those in video memory.
// GLOBAL: HYPERX 0x004cd4b8
extern SurfacePoint g_ptSurfacePixels;
// Its initialiser thunk, which heads the unit texture.cpp's code at 0x00436db0 belongs to.
// SYNTHETIC: HYPERX 0x00436d90
// $E2

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

// GLOBAL: HYPERX 0x004be6b8
extern unsigned int g_dwDisplayBpp;

// GLOBAL: HYPERX 0x004ce624
extern int g_nSurfaceMemoryUsed;

// GLOBAL: HYPERX 0x004ce618
extern int g_bBackBufferColourKey;

// GLOBAL: HYPERX 0x004ce62c
extern int g_hrLastResult;

// GLOBAL: HYPERX 0x004cef50
extern unsigned int g_dwFrameGameLogicStart;

struct SoftTexImage;
struct ClipVertex;
struct DrawPolySurface;

// The texture DLL's two entry points, loaded by name.
typedef void(__cdecl* MdmDrawTexPolyFn)(SoftTexImage* pTexture, ClipVertex* pVertices, int nCount, int nEdges);
typedef void(__cdecl* MdmInitDrawPolyFn)(DrawPolySurface* pSurface, int nOriginX, int nOriginY);

// GLOBAL: HYPERX 0x004ced58
extern MdmDrawTexPolyFn g_pfnMdmDrawTexPoly;

// GLOBAL: HYPERX 0x004ced60
extern MdmInitDrawPolyFn g_pfnMdmInitDrawPoly;

// GLOBAL: HYPERX 0x004b2f74
extern const float g_flOneNinetyNinth;

// GLOBAL: HYPERX 0x004b3e20
extern const float g_flLightAzimuth180;

// GLOBAL: HYPERX 0x004b3e24
extern const float g_flLightAzimuth54;

// GLOBAL: HYPERX 0x004b3e28
extern const float g_flLightAzimuth303;

// GLOBAL: HYPERX 0x004d0a00
extern Vec3 g_vLightDirExtra;

// GLOBAL: HYPERX 0x004b4f00
extern const GUID g_iidTextureSurface;

// The head of the software texture list, threaded through each texture's m_pNext.
// GLOBAL: HYPERX 0x004d2b78
extern class SoftTex* g_pSoftTexHead;

// Saved before SetDisplayMode so DDraw::ChangeDisplayMode(1) can restore it.
// GLOBAL: HYPERX 0x004ce61c
extern SavedDisplayMetrics* g_pSavedDesktopMode;

// The 8-bit palette made from the game palette when a D3D device exists.
// GLOBAL: HYPERX 0x004ce628
extern struct IDirectDrawPalette* g_pDevicePalette;

// The render states D3DRend::SetupRenderStates replays, numbered as in the SDK's d3dtypes.h.
// GLOBAL: HYPERX 0x004cbb88
extern int g_nRenderStateZEnable;

// GLOBAL: HYPERX 0x004cbb90
extern int g_nRenderStateShadeMode;

// GLOBAL: HYPERX 0x004cbb94
extern int g_nRenderStateTextureFilter;

// GLOBAL: HYPERX 0x004cbb98
extern int g_nRenderStateTextureMapBlend;

// GLOBAL: HYPERX 0x004cbb9c
extern int g_nRenderStateFillMode;

// GLOBAL: HYPERX 0x004cbba0
extern int g_nRenderStateCullMode;

// GLOBAL: HYPERX 0x004cbba4
extern int g_nRenderStateDither;

// GLOBAL: HYPERX 0x004cbba8
extern int g_nRenderStateSpecular;

// GLOBAL: HYPERX 0x004cbbac
extern int g_nRenderStateAntiAlias;

// GLOBAL: HYPERX 0x004cbbb0
extern int g_nRenderStateFog;

// GLOBAL: HYPERX 0x004cbbb4
extern int g_nRenderStateFogColour;

// GLOBAL: HYPERX 0x004cbbb8
extern int g_nRenderStateFogTableMode;

// GLOBAL: HYPERX 0x004cbbbc
extern float g_flRenderStateFogStart;

// GLOBAL: HYPERX 0x004cbbc0
extern float g_flRenderStateFogEnd;

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

// GLOBAL: HYPERX 0x004cd49c
extern int g_bExecuteSuppressed;

// The visible faces DrawFaces records and the list it sorts and draws them through.
// GLOBAL: HYPERX 0x004d0650
extern struct FaceRecord** g_ppFaceList;
// GLOBAL: HYPERX 0x004d0654
extern struct FaceRecord* g_pFaceRecords;

// The display format's colour key pixel, and the scroll and fill colours a new texture takes.
// GLOBAL: HYPERX 0x004be3e0
extern int g_nKeyColour;

// GLOBAL: HYPERX 0x004be3e4
extern int g_nDefaultScrollColour;

// GLOBAL: HYPERX 0x004be3e8
extern int g_nDefaultFillColour;

struct IDirectDraw;
struct IDirectDrawSurface;
struct IDirectDrawClipper;
class CDDTextSurface;

// The mode Direct3D chose, which is not the desktop's.
struct DisplayModeSize
{
    int nWidth;
    int nHeight;
};

// GLOBAL: HYPERX 0x004cda10
extern DisplayModeSize g_displayMode;

// DDraw::ClearDeviceState clears all 0x2d6 words with one rep stosd.
struct DDrawDeviceBlock
{
    IDirectDraw*        pDDrawObject;
    IDirectDrawSurface* pDDrawPrimary;
    IDirectDrawSurface* pBackSurface;
    // Released once on teardown, unlike the surface after it.
    IDirectDrawSurface* pZoomSurface;
    IDirectDrawSurface* pOffscreenSurface;
    int                 nUnusedA;
    // Released until its count reaches zero, so something else holds references.
    IDirectDrawClipper* pDDrawClipper;
    int                 nUnusedB;
    CDDTextSurface*     pFpsText;
    int                 anUnusedC[0x204];
    int                 nDisplayWidth;
    int                 nDisplayHeight;
    int                 nDisplayBpp;
    int                 nUnusedD;
    // Written and never read.
    unsigned int nDDrawDriverSurfaceCaps;
    GUID         ddrawDriverGuid;
    // Clear when a secondary DirectDraw device with 3D hardware is in use.
    int bIsPrimary;
    // The whole capability block of that driver, copied out of the enumeration's stack copy.
    DDrawCaps ddrawDriverCaps;
    // The emulation layer's block, copied out beside the driver's.
    DDrawCaps ddrawHelCaps;
    // D3DDevice::Init's result: one, or a release result, never an address.
    int bD3DActive;
};

// GLOBAL: HYPERX 0x004cdaa8
extern DDrawDeviceBlock g_ddrawDevice;

#endif
