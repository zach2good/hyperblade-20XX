#include "common/compat.h"
#include "globals.h"
#include "graphics/renderstate.h"
#include "graphicsglobals.h"
#include "d3ddevice.h"
#include "d3dshapes.h"
#include "app/gameapp.h"
#include "graphics/d3drend.h"
#include "graphics/d3dviewport.h"

// The scales are the dimensions halved, so both maxima come out one.
// FUNCTION: HYPERX 0x004316f0
int __cdecl D3DViewport::Setup(IDirect3DViewport** ppViewport, IDirect3DDevice* pDevice, int nWidth, int nHeight, int nX, int nY)
{
    D3DVIEWPORT desc;

    g_hrD3DResult = g_d3dState.pDirect3D->CreateViewport(ppViewport, 0);

    if (g_hrD3DResult != 0)
    {
        // STRING: HYPERX 0x004bd628
        g_gameApp.Error("Direct3D CreateViewport failed.\n%s",
                        DDraw::ErrorToString(g_hrD3DResult));

        return 0;
    }

    g_hrD3DResult = pDevice->AddViewport(*ppViewport);

    if (g_hrD3DResult != 0)
    {
        // STRING: HYPERX 0x004bd608
        g_gameApp.Error("Direct3D AddViewport failed.\n%s",
                        DDraw::ErrorToString(g_hrD3DResult));

        return 0;
    }

    memset(&desc, 0, sizeof(desc));

    desc.dwSize                                             = sizeof(desc);
    desc.dwX                                                = nX;
    desc.dwY                                                = nY;
    desc.dwWidth                                            = nWidth;
    desc.dwHeight                                           = nHeight;
    desc.dvScaleX                                           = (float)(unsigned int)nWidth * 0.5f;
    desc.dvScaleY                                           = (float)(unsigned int)nHeight * 0.5f;
    g_d3dState.aViewportRects[g_d3dState.nViewportCount].x1 = nX;
    g_d3dState.aViewportRects[g_d3dState.nViewportCount].y1 = nY;
    g_d3dState.aViewportRects[g_d3dState.nViewportCount].x2 = nX + nWidth;
    g_d3dState.aViewportRects[g_d3dState.nViewportCount].y2 = nY + nHeight;

    g_d3dState.nViewportCount++;

    double dDivisor;

    dDivisor    = desc.dvScaleX + desc.dvScaleX;
    desc.dvMaxX = (float)((float)(unsigned int)nWidth / dDivisor);
    dDivisor    = desc.dvScaleY + desc.dvScaleY;
    desc.dvMaxY = (float)((float)(unsigned int)nHeight / dDivisor);

    g_hrD3DResult = (*ppViewport)->SetViewport(&desc);

    if (g_hrD3DResult != 0)
    {
        // STRING: HYPERX 0x004bd5e8
        g_gameApp.Error("Direct3D SetViewport failed.\n%s",
                        DDraw::ErrorToString(g_hrD3DResult));

        return 0;
    }

    return 1;
}

// The width rounds up to a multiple of four; the height does not.
// FUNCTION: HYPERX 0x00431880
int __cdecl D3DViewport::SetForTexture(unsigned int nViewportId, Texture* pSource)
{
    Point extent = pSource->GetSize();
    Point inset  = pSource->GetInset();

    extent.x = (extent.x - inset.x * 2 + 3) & ~3;
    extent.y -= inset.y * 2;

    Point origin = pSource->GetPosition();

    origin.Offset(pSource->GetInset());

    if (D3DViewport::Setup(&g_d3dState.apViewports[nViewportId], g_d3dState.pDevice, extent.x, extent.y, origin.x, origin.y) == 0)
    {
        return 0;
    }

    if (D3DRend::SetupRenderStates(g_d3dState.pDevice, g_d3dState.apViewports[nViewportId], g_nRenderStateTexturePerspective) == 0)
    {
        return 0;
    }

    g_hrD3DResult = g_d3dState.apViewports[0]->SetBackground(g_d3dState.hBackgroundMaterial);

    return g_hrD3DResult == 0;
}

// FUNCTION: HYPERX 0x00432370
int __stdcall D3DViewport::Clear(void)
{
    g_hrD3DResult = g_d3dState.apViewports[0]->Clear(1, g_d3dState.aViewportRects, D3DCLEAR_TARGET);

    return g_hrD3DResult == 0;
}
