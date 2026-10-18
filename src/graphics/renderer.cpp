#include "common/compat.h"
#include "graphics/hwrenderer.h"
#include "graphics/presenter.h"
#include "renderer.h"
#include "colorpalette.h"
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
#include "math/transformobject.h"
#include "projrender.h"
#include <cstring>
#include "engine/engineobject.h"
#include "renderoptions.h"
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
        const int dr    = static_cast<int>(g_aSoftTexPalette[i].peRed) - rTarget;
        const int dg    = static_cast<int>(g_aSoftTexPalette[i].peGreen) - gTarget;
        const int db    = static_cast<int>(g_aSoftTexPalette[i].peBlue) - bTarget;
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

#define DROME_TEXTURED_DEFAULT 1

// A network or loading frame relights the first slots from fixed directions first.
// FUNCTION: HYPERX 0x00442d20
void Renderer::DrawAllMeshes()
{
    MeshBone* pBone;

    EngineObject* pDrome = g_pGameEngine->m_ppSlots[SLOT_DROME];

    int nBone = 0;

    if (g_bDromeIniDirty != 0 && g_pGameOptions != nullptr)
    {
        g_nDromeTextured = g_pGameOptions->ReadInt(g_pszDromeSection,
                                                   g_pszDromeTexturedKey,
                                                   DROME_TEXTURED_DEFAULT);

        g_bDromeIniDirty = 0;
        g_bLoadingMode   = 1;
    }

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
                pBone->CalcVertexColors();
            }
        }

        g_lightDirs[0].dir = g_lightDirs[1].dir;

        nRelitBone = static_cast<short>(nBone);

        nBone++;

        pBone = &pDrome->m_pBones[nRelitBone];

        if (pBone->IsBound() != 0)
        {
            pBone->CalcVertexColors();
        }

        g_lightDirs[0].dir = g_lightDirs[2].dir;

        pBone = &pDrome->m_pBones[static_cast<short>(nBone)];

        if (pBone->IsBound() != 0)
        {
            pBone->CalcVertexColors();
        }

        g_lightDirs[0].dir.z = vSavedDir.z;
        g_lightDirs[0].dir.x = vSavedDir.x;
        g_lightDirs[0].dir.y = vSavedDir.y;

        g_bDromePass = 0;
    }

    nBone = 0;

    while (nBone < pDrome->GetBoneCount())
    {
        pBone = &pDrome->m_pBones[static_cast<short>(nBone)];

        if (pBone->IsBound() != 0)
        {
            pBone->DrawFaces();
        }

        nBone++;
    }

    g_bLoadingMode = 0;
}

// FUNCTION: HYPERX 0x00442f40
void Renderer::SetLoadingMode()
{
    g_bLoadingMode = 1;
}

// FUNCTION: HYPERX 0x00442f50
int Renderer::SetDromeTextured(int nEnable)
{
    if (g_cMdmTexLoaded == 0 && !HwRenderer::Capturing())
    {
        return 0;
    }

    g_nDromeTextured = (nEnable != 0);

    Renderer::SetLoadingMode();

    if (g_pGameOptions != nullptr)
    {
        g_pGameOptions->WriteInt(g_pszDromeSection, g_pszDromeTexturedKey, g_nDromeTextured);
    }

    return g_nDromeTextured;
}

// FUNCTION: HYPERX 0x00442fa0
int Renderer::ToggleDromeTextured()
{
    return Renderer::SetDromeTextured(g_nDromeTextured == 0);
}

// FUNCTION: HYPERX 0x0044b2c0
void Renderer::DrawWholeScene()
{
    DrawPolySurface surface;

    if (g_pfnMdmInitDrawPoly != nullptr)
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

// FUNCTION: HYPERX 0x0044bc30
int Renderer::PresentFrame()
{
    // The splash owns the screen, so the start-up timing frames are drawn but not shown.
    if (g_nSplashFrame != 0)
    {
        return 1;
    }

    Presenter::Frame(g_pBackBufferBitsCopy, g_nBackBufferWidth, g_nBackBufferWidth, g_nBackBufferHeight, g_bmiSoftBackBuffer.bmiColors);

    return 1;
}

// FUNCTION: HYPERX 0x0044bc80
void Renderer::DrawSceneDispatch()
{
    Renderer::DrawWholeScene();
}

// FUNCTION: HYPERX 0x0044be80
RenderOptions Renderer::GetRenderOptions()
{
    return g_renderOptions;
}

// FUNCTION: HYPERX 0x0044beb0
void Renderer::SetRenderOption(RenderOption nOption, int nValue)
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

// FUNCTION: HYPERX 0x0044bf60
void Renderer::ClearFrame()
{
    if (g_pBackBufferBitsCopy != nullptr)
    {
        const auto nIndex = [&]
        {
            if (g_pColorPalette != nullptr)
            {
                return g_pColorPalette->NearestIndex(CLEAR_COLOUR, -1);
            }

            return NearestSoftPaletteIndex(CLEAR_COLOUR);
        }();

        memset(g_pBackBufferBitsCopy, nIndex, g_nBackBufferWidth * g_nBackBufferHeight);
    }
}

// FUNCTION: HYPERX 0x0047a980
void Renderer::SetCameraTransformAndViewport(LayoutState block)
{
    g_cameraViewportBlock = block;
    g_pBackBufferBitsCopy = static_cast<unsigned char*>(block.pBits);
    g_pCameraTransform    = block.pCamera;
    g_projRenderParams    = block.pCamera->m_clip;

    g_nPrevWidth            = block.nPrevWidth;
    g_nPrevHeight           = block.nPrevHeight;
    g_nSurfacePitchCopy     = block.nPitch;
    g_nBackBufferPixelCount = block.nByteSize;
    g_nViewportOriginX      = block.nHalfWidth;
    g_nViewportOriginY      = block.nHalfHeight;
}

// Azimuths 303, 54 and 180 degrees; the first fills two table entries.
// FUNCTION: HYPERX 0x0047aa20
void Renderer::InitializeLightDirections()
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
    vSpherical2.z    = static_cast<float>(dDegToRad * g_flLightAzimuth54);

    g_lightDirs[2].dir = Math::SphericalToCartesian(&vSpherical2);

    vSpherical3.x = g_flHalfPi;
    vSpherical3.y = g_flOne;
    dDegToRad     = g_flDegToRad;
    vSpherical3.z = static_cast<float>(dDegToRad * g_flLightAzimuth180);

    g_vLightDirExtra = Math::SphericalToCartesian(&vSpherical3);
}

int g_nDromeTextured = 1;

// STRING: HYPERX 0x004c1ea8
const char* g_pszDromeSection = "Misc";

// STRING: HYPERX 0x004c1e98
const char* g_pszDromeTexturedKey = "DromeTextured";
int         g_bDromeIniDirty      = 1;
