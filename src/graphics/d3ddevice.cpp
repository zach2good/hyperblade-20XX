#include "common/compat.h"
#include "cddbitmap.h"
#include "engine/cameraobject.h"
#include "globals.h"
#include "graphics/graphicsglobals.h"
#include "graphics/renderstate.h"
#ifdef HYPERX_BRINGUP
#include "app/bringup.h"
#endif
#include "d3ddevice.h"
#include "app/gameapp.h"
#include "d3dshapes.h"
#include "d3ddeviceinit.h"
#include "graphics/d3dapi.h"
#include "graphics/d3drend.h"
#include "graphics/ddrawapi.h"
#include "graphics/display.h"
#include "d3dviewport.h"

// FUNCTION: HYPERX 0x004314d0
int __cdecl D3DDevice::Init(IDirectDrawSurface* pSurface)
{
    if (g_d3dState.pDirect3D != 0)
    {
        if (g_d3dState.bOnlyEmulation != 0 || g_d3dState.aD3DDrivers[g_d3dState.nD3DDriverIndex].bIsHardware != 0)
        {
            g_hrD3DResult = pSurface->QueryInterface(g_d3dState.aD3DDrivers[g_d3dState.nD3DDriverIndex].guid,
                                                     (void**)&g_d3dState.pDevice);

            if (g_hrD3DResult != 0)
            {
                // STRING: HYPERX 0x004bd594
                g_gameApp.Fatal("QueryInterface for Direct3D Driver failed!\nChange your video mode to 16 bpp.\n%s\n", DDraw::ErrorToString(g_hrD3DResult));

                return D3DRend::ReleaseAllBuffers();
            }

            g_d3dState.bD3DTexturesEnabled = 0;

            if (g_d3dState.aD3DDrivers[g_d3dState.nD3DDriverIndex].bDoesTextures != 0)
            {
                int nFormat = -1;

                g_hrD3DResult = g_d3dState.pDevice->EnumTextureFormats(
                    D3D::EnumTextureFormatsCallback, &nFormat);

                if (g_hrD3DResult != 0)
                {
                    g_d3dState.nTextureFormat = -1;
                }
                else
                {
                    g_d3dState.bD3DTexturesEnabled = 1;
                    g_cMdmTexLoaded                = 1;
                    if ((g_d3dState.aD3DDrivers[g_d3dState.nD3DDriverIndex].deviceDesc.dpcTriCaps.dwTextureFilterCaps & D3DPTFILTERCAPS_LINEAR) != 0)
                    {
                        g_nRenderStateTextureFilter = D3DFILTER_LINEAR;
                    }
                    else
                    {
                        g_nRenderStateTextureFilter = D3DFILTER_NEAREST;
                    }

                    g_d3dState.nTextureFormat = nFormat;
                }
            }
            else
            {
                g_d3dState.nTextureFormat = -1;
            }

            Display::ApplyRenderOptions(g_gameApp.m_hMenu, g_d3dState.bD3DTexturesEnabled, g_nRenderStateTextureFilter == D3DFILTER_LINEAR);

#ifdef HYPERX_BRINGUP
            // The main viewport is sized from the mode entry, so HB_RES has to reach it separately.
            {
                int nViewW = g_displayState.aDisplayModes[g_displayState.nDisplayModeIndex].nWidth;
                int nViewH = g_displayState.aDisplayModes[g_displayState.nDisplayModeIndex].nHeight;

                BringUp::OverrideDisplayMode(&nViewW, &nViewH);

                if (D3DViewport::Setup(&g_d3dState.apViewports[0], g_d3dState.pDevice, nViewW, nViewH, 0, 0) != 0
#else
            if (D3DViewport::Setup(&g_d3dState.apViewports[0], g_d3dState.pDevice, g_displayState.aDisplayModes[g_displayState.nDisplayModeIndex].nWidth, g_displayState.aDisplayModes[g_displayState.nDisplayModeIndex].nHeight, 0, 0) != 0
#endif
                    && D3DRend::SetupRenderStates(g_d3dState.pDevice, g_d3dState.apViewports[0], g_nRenderStateTexturePerspective) != 0 && D3DRend::CreateMaterial() != 0)
                {
                    D3DRend::InitBatchPools();

                    return 1;
                }
#ifdef HYPERX_BRINGUP
            }
#endif
        }
    }
    else
    {
        Display::ApplyRenderOptions(g_gameApp.m_hMenu, 0, 0);
        g_cMdmTexLoaded = 1;
    }

    return D3DRend::ReleaseAllBuffers();
}

// FUNCTION: HYPERX 0x00431c40
int __cdecl D3DDevice::SelectDriverAndMode(int* pnDriver, int* pnMode)
{
    unsigned int nMask;

    int nDriver = *pnDriver;
    int nMode   = *pnMode;

    if (nMode == MODE_USE_WINDOW && g_d3dState.bIsPrimary == 0)
    {
        g_gameApp.Error(
            // STRING: HYPERX 0x004bd818
            "Cannot render to a window when the DirectDraw device is not the primary.\n");

        return 0;
    }

    if (nDriver == DRIVER_ANY)
    {
        if (nMode == MODE_USE_WINDOW)
        {
            if (!D3D::FindDeviceByFormat(&nDriver, D3D::BppToDDBD(g_d3dState.nDesktopBpp)))
            {
                return 0;
            }

            if (nDriver == SELECT_NONE)
            {
                g_gameApp.Error(
                    // STRING: HYPERX 0x004bd7c0
                    "Cannot find a D3D device driver which is compatible with the current display depth.\n");

                return 0;
            }

            *pnDriver = nDriver;
            *pnMode   = nMode;

            return 1;
        }

        if (nMode == MODE_ANY)
        {
            if (g_d3dState.bIsPrimary != 0)
            {
                nMask = D3D::BppToDDBD(g_d3dState.nDesktopBpp);
            }
            else
            {
                nMask = 0;
            }

            for (int i = 0; i < g_displayState.nDisplayModeCount; i++)
            {
                nMask |= D3D::BppToDDBD(g_displayState.aDisplayModes[i].nBpp);
            }

            if (!D3D::FindDeviceByFormat(&nDriver, nMask))
            {
                return 0;
            }

            if (nDriver == SELECT_NONE)
            {
                g_gameApp.Error(
                    // STRING: HYPERX 0x004bd748
                    "Cannot find a D3D device driver which is compatible with the current display depth or any supported fullscreen mode.\n");
            }
        }
        else
        {
            if (!D3D::FindDeviceByFormat(&nDriver,
                                         D3D::BppToDDBD(
                                             g_displayState.aDisplayModes[nMode].nBpp)))
            {
                return 0;
            }

            if (nDriver == SELECT_NONE)
            {
                g_gameApp.Error(
                    // STRING: HYPERX 0x004bd6ec
                    "Cannot find a D3D device driver which is compatible with the specified fullscreen mode.\n");

                return 0;
            }

            *pnDriver = nDriver;
            *pnMode   = nMode;

            return 1;
        }
    }

    if (nMode == MODE_ANY)
    {
        if (g_d3dState.bIsPrimary != 0 && (g_d3dState.aD3DDrivers[nDriver].deviceDesc.dwDeviceRenderBitDepth & D3D::BppToDDBD(g_d3dState.nDesktopBpp)) != 0)
        {
            *pnDriver = nDriver;
            *pnMode   = MODE_USE_WINDOW;

            return 1;
        }

        if (!D3D::FindDisplayMode(&nMode, g_d3dState.aD3DDrivers[nDriver].deviceDesc.dwDeviceRenderBitDepth))
        {
            return 0;
        }

        if (nMode == SELECT_NONE)
        {
            g_gameApp.Error(
                // STRING: HYPERX 0x004bd678
                "The selected D3D device driver is not compatible with the current display depth or any supported fullscreen modes.\n");

            return 0;
        }

        *pnDriver = nDriver;
        *pnMode   = nMode;

        return 1;
    }

    if (nMode == MODE_USE_WINDOW)
    {
        if ((g_d3dState.aD3DDrivers[nDriver].deviceDesc.dwDeviceRenderBitDepth & D3D::BppToDDBD(g_d3dState.nDesktopBpp)) != 0)
        {
            *pnDriver = nDriver;
            *pnMode   = nMode;

            return 1;
        }

        nMode = MODE_ANY;

        if (!D3DDevice::SelectDriverAndMode(&nDriver, &nMode))
        {
            return 0;
        }

        if (nDriver == SELECT_NONE)
        {
            return 0;
        }

        *pnDriver = nDriver;
        *pnMode   = nMode;

        return 1;
    }
    else
    {
        if ((g_d3dState.aD3DDrivers[nDriver].deviceDesc.dwDeviceRenderBitDepth & D3D::BppToDDBD(g_displayState.aDisplayModes[nMode].nBpp)) != 0)
        {
            *pnDriver = nDriver;
            *pnMode   = nMode;

            return 1;
        }

        nMode = MODE_ANY;

        if (!D3DDevice::SelectDriverAndMode(&nDriver, &nMode))
        {
            return 0;
        }

        if (nDriver == SELECT_NONE)
        {
            return 0;
        }

        *pnDriver = nDriver;
        *pnMode   = nMode;

        return 1;
    }

    *pnDriver = nDriver;
    *pnMode   = nMode;

    return 1;
}

// FUNCTION: HYPERX 0x00432810
int __stdcall D3DDevice::BeginScene(void)
{
    g_hrD3DResult = g_d3dState.pDevice->BeginScene();

    if (g_hrD3DResult != 0)
    {
        D3DRend::ResetBatchBuffers(1);

        return g_hrD3DResult;
    }

    return 0;
}

// FUNCTION: HYPERX 0x00432840
int __stdcall D3DDevice::EndDeviceScene(void)
{
    D3DRend::FlushBatch(1);

    g_hrD3DResult = g_d3dState.pDevice->EndScene();

    if (g_hrD3DResult != 0)
    {
        D3DRend::ResetBatchBuffers(1);

        return g_hrD3DResult;
    }

    return 0;
}

// Answers nought whatever happened.
// FUNCTION: HYPERX 0x0043d850
int __stdcall D3DDevice::EndScene(void)
{
    if (g_ddrawDevice.bD3DActive != 0)
    {
        D3DDevice::EndDeviceScene();
    }

    if (g_pScene != 0 && g_pScene->m_pTarget != 0)
    {
        g_pScene->m_pTarget->Draw();

        if (g_hudSurfaces.pReticle != 0)
        {
            g_hudSurfaces.pReticle->Draw();
        }
    }

    return 0;
}
