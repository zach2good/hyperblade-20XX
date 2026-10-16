#include "common/compat.h"
#include "graphics/hwrenderer.h"
#include "graphics/presenter.h"
#include "renderer.h"
#include "globals.h"
#include "app/gameapp.h"
#include "engine/objectclasses.h"
#include "engine/cameraobject.h"
#include "engine/cameraprojectionstate.h"
#include "meshbone.h"
#include "graphicsglobals.h"
#include "ui/hudglobals.h"
#include "engine/gameoptions.h"
#include "engine/devicector.h"
#include "engine/gameengine.h"
#include "d3ddevice.h"
#include "math/transformobject.h"
#include "projrender.h"
#include <string.h>
#include "engine/engineobject.h"
#include "renderoptions.h"
#include "graphics/ddrawapi.h"
#include "ui/debughud.h"
#include "physics/collisionellipsoid.h"

namespace
{

// The software palette entry nearest a 0xRRGGBB colour.
int NearestSoftPaletteIndex(unsigned int nColour)
{
    const int rTarget = (nColour >> 16) & 0xff;
    const int gTarget = (nColour >> 8) & 0xff;
    const int bTarget = nColour & 0xff;

    auto nBest     = 0;
    auto nBestDist = 0x7fffffff;

    for (int i = 0; i < 256; i++)
    {
        const int dr    = (int)g_aSoftTexPalette[i].peRed - rTarget;
        const int dg    = (int)g_aSoftTexPalette[i].peGreen - gTarget;
        const int db    = (int)g_aSoftTexPalette[i].peBlue - bTarget;
        const int nDist = dr * dr + dg * dg + db * db;

        if (nDist < nBestDist)
        {
            nBestDist = nDist;
            nBest     = i;
        }
    }

    return nBest;
}

} // namespace

// The surface the texture DLL is told to draw into.
struct DrawPolySurface
{
    int   nWidth;  // 0x00
    int   nHeight; // 0x04
    int   nPitch;  // 0x08
    int   nDepth;  // 0x0c
    void* pBits;   // 0x10
};

#define DRAW_POLY_DEPTH 8

extern int g_bDromePass;

// GLOBAL: HYPERX 0x004c1e94
extern int g_bDromeIniDirty;

// The arena pass substitutes the ini's answer for its duration.
extern int g_nMdmTexturesReady;

#define DROME_TEXTURED_DEFAULT 1

// Only a null target clears first. The answer is always nought.
// FUNCTION: HYPERX 0x004337b0
int __cdecl Renderer::RenderFrame(const Texture* pTarget)
{
    if (pTarget == 0)
    {
        DDraw::ClearBackBuffer(CLEAR_COLOUR);
    }

    Renderer::DrawWholeScene();

    return 0;
}

// A network or loading frame relights the first slots from fixed directions first.
// FUNCTION: HYPERX 0x00442d20
void __cdecl Renderer::DrawAllMeshes(void)
{
    MeshBone* pBone;

    EngineObject* pDrome = g_pGameEngine->m_ppSlots[SLOT_DROME];

    int nBone = 0;

    if (g_bDromeIniDirty != 0 && g_pGameOptions != 0)
    {
        g_nDromeTextured = g_pGameOptions->ReadInt(g_pszDromeSection,
                                                   g_pszDromeTexturedKey,
                                                   DROME_TEXTURED_DEFAULT);

        g_bDromeIniDirty = 0;
        g_bLoadingMode   = 1;
    }

    const int nSavedTextured = g_nMdmTexturesReady;

    g_nMdmTexturesReady = g_nDromeTextured;

    if (g_bNetworkGame != 0 || g_bLoadingMode != 0)
    {
        short nRelitBone;
        Vec3  vSavedDir;

        pDrome->ApplyRenderSlotPose();

        vSavedDir = g_lightDirs[0].dir;

        g_bDromePass = 1;

        if (pDrome->GetBoneCount() > 2)
        {
            g_lightDirs[0].dir = g_vLightDirExtra;

            nBone = 1;

            pBone = pDrome->m_pBones;

            if (pBone->IsBound() != 0)
            {
                pBone->CalcVertexColors(g_nMdmTexturesReady);
            }
        }

        g_lightDirs[0].dir = g_lightDirs[1].dir;

        nRelitBone = (short)nBone;

        nBone++;

        pBone = &pDrome->m_pBones[nRelitBone];

        if (pBone->IsBound() != 0)
        {
            pBone->CalcVertexColors(g_nMdmTexturesReady);
        }

        g_lightDirs[0].dir = g_lightDirs[2].dir;

        pBone = &pDrome->m_pBones[(short)nBone];

        if (pBone->IsBound() != 0)
        {
            pBone->CalcVertexColors(g_nMdmTexturesReady);
        }

        g_lightDirs[0].dir.z = vSavedDir.z;
        g_lightDirs[0].dir.x = vSavedDir.x;
        g_lightDirs[0].dir.y = vSavedDir.y;

        g_bDromePass = 0;
    }

    nBone = 0;

    while (nBone < pDrome->GetBoneCount())
    {
        pBone = &pDrome->m_pBones[(short)nBone];

        if (pBone->IsBound() != 0)
        {
            pBone->DrawFaces();
        }

        nBone++;
    }

    g_nMdmTexturesReady = nSavedTextured;

    g_bLoadingMode = 0;
}

// FUNCTION: HYPERX 0x00442f40
void __cdecl Renderer::SetLoadingMode(void)
{
    g_bLoadingMode = 1;
}

// FUNCTION: HYPERX 0x00442f50
int __cdecl Renderer::SetDromeTextured(int nEnable)
{
    if (g_cMdmTexLoaded == 0 && !HwRenderer::Capturing())
    {
        return 0;
    }

    g_nDromeTextured = (nEnable != 0);

    Renderer::SetLoadingMode();

    if (g_pGameOptions != 0)
    {
        g_pGameOptions->WriteInt(g_pszDromeSection, g_pszDromeTexturedKey, g_nDromeTextured);
    }

    return g_nDromeTextured;
}

// FUNCTION: HYPERX 0x00442fa0
int __cdecl Renderer::ToggleDromeTextured(void)
{
    return Renderer::SetDromeTextured(g_nDromeTextured == 0);
}

// FUNCTION: HYPERX 0x0044b2c0
void __stdcall Renderer::DrawWholeScene(void)
{
    DrawPolySurface surface;

    if (g_pfnMdmInitDrawPoly != 0)
    {
        surface.nWidth  = g_nBackBufferWidth;
        surface.nHeight = g_nBackBufferHeight;
        surface.nPitch  = g_nSurfacePitchCopy;
        surface.pBits   = g_pBackBufferBitsCopy;
        surface.nDepth  = DRAW_POLY_DEPTH;

        g_pfnMdmInitDrawPoly(&surface, g_nViewportOriginX, g_nViewportOriginY);
    }

    if (g_bNetworkGame != 0 && g_bShowLightingInfo != 0)
    {
        DebugHUD::UpdateLightingInfo();
    }

    Renderer::DrawAllMeshes();

    g_pScene->DrawShadowAndTransparentPass(&g_arenaEllipsoid);
}

// FUNCTION: HYPERX 0x0044b7c0
void __stdcall Renderer::DestroyBackBuffer(void)
{
    if (g_cUseSoftRenderer == 0)
    {
        DDraw::DestroyAllSurfaces();

        return;
    }

    if (g_hSoftBackDC != 0)
    {
        SelectObject(g_hSoftBackDC, g_hSoftBackOldBitmap);

        DeleteObject(g_hSoftBackBitmap);

        DeleteDC(g_hSoftBackDC);

        g_hSoftBackDC = 0;
    }
}

// The flip is skipped while the cursor shows.
// FUNCTION: HYPERX 0x0044bc30
int __stdcall Renderer::PresentFrame(void)
{
    // The splash owns the screen, so the start-up timing frames are drawn but not shown.
    if (g_nSplashFrame != 0)
    {
        return 1;
    }

    if (g_cUseSoftRenderer == 0)
    {
        if (g_bCursorHidden == 0)
        {
            return 1;
        }

        return DDraw::FlipOrBlt();
    }

    // GDI drew the palette into the DIB's colour table; the GPU applies it.
    RGBQUAD aColours[256] = {};

    GdiFlush();
    GetDIBColorTable(g_hSoftBackDC, 0, 256, aColours);

    Presenter::Frame(g_pBackBufferBitsCopy, g_nBackBufferWidth, g_nBackBufferWidth, g_nBackBufferHeight, aColours);

    return 1;
}

// FUNCTION: HYPERX 0x0044bc80
void __stdcall Renderer::DrawSceneDispatch(void)
{
    if (g_cUseSoftRenderer == 0)
    {
        DDraw::PaintSurface();
    }
    else
    {
        Renderer::DrawWholeScene();
    }
}

// FUNCTION: HYPERX 0x0044be80
RenderOptions __cdecl Renderer::GetRenderOptions()
{
    return g_renderOptions;
}

// FUNCTION: HYPERX 0x0044beb0
void __cdecl Renderer::SetRenderOption(RenderOption nOption, int nValue)
{
    switch (nOption)
    {
        case RENDER_OPTION_PERSPECTIVE:
            g_renderOptions.bPerspective = nValue;

            // Falls through

        case RENDER_OPTION_FILTERED:
            g_renderOptions.bFiltered = nValue;
            break;

        case RENDER_OPTION_TEXTURED:
            g_renderOptions.bTextured = nValue;
            break;
    }
}

// GLOBAL: HYPERX 0x004d0a28
class TransformObject* g_pCameraTransform;

// The back buffer lock writes pBits and nPitch.
// GLOBAL: HYPERX 0x004d0a30
LayoutState g_cameraViewportBlock;

// GLOBAL: HYPERX 0x004d1a68
ClipExtents g_projRenderParams;

// The software path has nothing to bracket and reports success.
// FUNCTION: HYPERX 0x0044bf00
int __stdcall Renderer::BeginScene(void)
{
    int nResult;

    if (g_cUseSoftRenderer == 0)
    {
        if (g_ddrawDevice.bD3DActive != 0)
        {
            nResult = D3DDevice::BeginScene();
        }
        else
        {
            nResult = 0;
        }

        return nResult == 0;
    }

    return 1;
}

// FUNCTION: HYPERX 0x0044bf40
int __stdcall Renderer::EndScene(void)
{
    if (g_cUseSoftRenderer == 0)
    {
        return D3DDevice::EndScene() == 0;
    }

    return 1;
}

// FUNCTION: HYPERX 0x0044bf60
void __cdecl Renderer::ClearFrame(void)
{
    if (g_cUseSoftRenderer == 0)
    {
        DDraw::ClearBackBuffer(CLEAR_COLOUR);

        return;
    }

    if (g_pBackBufferBitsCopy != 0)
    {
        int nIndex = GetNearestPaletteIndex(g_hDisplayPalette, CLEAR_COLOUR);

        // GetNearestPaletteIndex answers CLR_INVALID once the display palette is gone.
        if (nIndex == -1 || (unsigned int)nIndex >= 256)
        {
            nIndex = NearestSoftPaletteIndex(CLEAR_COLOUR);
        }

        memset(g_pBackBufferBitsCopy, nIndex, g_nBackBufferWidth * g_nBackBufferHeight);
    }
}

// Under Direct3D the second and fourth projection floats gain a pixel.
// FUNCTION: HYPERX 0x0047a980
void __cdecl Renderer::SetCameraTransformAndViewport(LayoutState block)
{
    g_cameraViewportBlock = block;
    g_pBackBufferBitsCopy = (unsigned char*)block.pBits;
    g_pCameraTransform    = block.pCamera;
    g_projRenderParams    = block.pCamera->m_clip;

    if (g_ddrawDevice.bD3DActive != 0)
    {
        g_projRenderParams.flMaxX += 1.0f;
        g_projRenderParams.flMaxY += 1.0f;
    }

    g_nPrevWidth            = block.nPrevWidth;
    g_nPrevHeight           = block.nPrevHeight;
    g_nSurfacePitchCopy     = block.nPitch;
    g_nBackBufferPixelCount = block.nByteSize;
    g_nViewportOriginX      = block.nHalfWidth;
    g_nViewportOriginY      = block.nHalfHeight;
}

// Azimuths 303, 54 and 180 degrees; the first fills two table entries.
// FUNCTION: HYPERX 0x0047aa20
void __stdcall Renderer::InitializeLightDirections(void)
{
    Vec3 vSpherical2;
    Vec3 vSpherical3;

    g_gameApp.m_vLightSpherical.x = g_flHalfPi;
    g_gameApp.m_vLightSpherical.y = g_flOne;
    g_gameApp.m_vLightSpherical.z = g_flLightAzimuth303 * g_flDegToRad;

    g_lightDirs[0].dir = g_lightDirs[1].dir =
        Math::SphericalToCartesian(&g_gameApp.m_vLightSpherical);

    vSpherical2.x    = g_flHalfPi;
    vSpherical2.y    = g_flOne;
    double dDegToRad = g_flDegToRad;
    vSpherical2.z    = (float)(dDegToRad * g_flLightAzimuth54);

    g_lightDirs[2].dir = Math::SphericalToCartesian(&vSpherical2);

    vSpherical3.x = g_flHalfPi;
    vSpherical3.y = g_flOne;
    dDegToRad     = g_flDegToRad;
    vSpherical3.z = (float)(dDegToRad * g_flLightAzimuth180);

    g_vLightDirExtra = Math::SphericalToCartesian(&vSpherical3);
}

int g_nDromeTextured = 1;

// STRING: HYPERX 0x004c1ea8
const char* g_pszDromeSection = "Misc";

// STRING: HYPERX 0x004c1e98
const char* g_pszDromeTexturedKey = "DromeTextured";
int         g_bDromeIniDirty      = 1;
