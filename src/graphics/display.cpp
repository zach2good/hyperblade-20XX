#include "common/compat.h"
#include "globals.h"
#include "graphicsglobals.h"
#include "renderer.h"
#include "ui/infobarbase.h"
struct GameApp;
#include "app/window.h"
#include "graphics/graphics.h"
#include "graphics/ddrawapi.h"
#include "ui/hud.h"
#include "app/menuids.h"
#include <stdlib.h>
#include "app/gameapp.h"
#include "graphics/display.h"
#include "softrenderer.h"

extern int g_nDisplayHorzRes;

extern int g_nDisplayVertRes;

extern int g_nDisplayBitsPerPixel;

// The hardware path returns 0 or 1; the software path returns the back buffer resize result.
// FUNCTION: HYPERX 0x0044b500
int Display::Initialize(HWND hWnd, HDC hDisplayDC, HPALETTE hPalette)
{
    VideoMemory videoMemory;

    g_hScreenBltDC    = hDisplayDC;
    g_hDisplayPalette = hPalette;
    g_hWndGame        = hWnd;

    g_nDisplayWindowID = GetWindowLongA(hWnd, GWL_ID);

    Window::ApplyFullscreenStyle(g_hWndGame, 0);

    if (g_cUseSoftRenderer == 0)
    {
        if (DDraw::InitDeviceAndSurfaces(g_hDisplayPalette) != 0)
        {
            g_bBackBufferReady = DDraw::CreatePrimaryAndBackBuffer();

            if (g_bBackBufferReady != 0)
            {
                DDraw::GetVideoMemory(&videoMemory, g_pDDrawDevice);

                g_nSurfaceMemoryUsed += HUD::CreateSurfaces();

                DDraw::GetVideoMemory(&videoMemory, g_pDDrawDevice);

                if (g_pRenderer != 0)
                {
                    g_pRenderer->LayoutSurfaces();
                }

                if (g_bBackBufferReady != 0)
                {
                    if (g_cMdmTexLoaded == 0)
                    {
                        g_cMdmTexLoaded = (char)Graphics::LoadMdmTexDLL();

                        if (g_cMdmTexLoaded != 0)
                        {
                            Display::ApplyRenderOptions(g_gameApp.m_hMenu, g_cMdmTexLoaded & 0xff, 0);
                        }
                    }

                    return 0;
                }
            }

            return 1;
        }
    }

    g_cUseSoftRenderer = 1;
    g_hSoftBackDC      = 0;

    const HDC hdc = GetDC(0);

    if (hdc != 0)
    {
        g_hSoftBackDC = CreateCompatibleDC(hdc);
        ReleaseDC(0, hdc);
    }

    if (g_hSoftBackDC == 0)
    {
        // STRING: HYPERX 0x004c2d48
        g_gameApp.Fatal("Error allocating graphics device context!");
        exit(1);
    }

    g_cMdmTexLoaded = (char)Graphics::LoadMdmTexDLL();
    Display::ApplyRenderOptions(g_gameApp.m_hMenu, g_cMdmTexLoaded & 0xff, 0);

    int nWidth  = g_dwDisplayWidth;
    int nHeight = g_dwDisplayHeight;

    g_bBackBufferReady = SoftRenderer::ResizeBackBuffer(&nWidth, &nHeight);

    if (g_bFullScreen == 0)
    {
        // Not MAKELPARAM: the client adds the unmasked halves at 0x0044b6bb.
        SendMessageA(hWnd, WM_SIZE, 0, (g_dwDisplayHeight << 16) + g_dwDisplayWidth);
    }

    if (g_bBackBufferReady != 0 && g_pRenderer != 0)
    {
        g_pRenderer->LayoutSurfaces();
    }

    return g_bBackBufferReady;
}

// FUNCTION: HYPERX 0x0044b850
LONG __cdecl Display::ChangeColorDepth(int nMode)
{
    LONG         lResult = 0;
    HDC          hdc;
    unsigned int nBpp;
    unsigned int nWidth;
    unsigned int nHeight;
    unsigned int nCfgWidth;
    unsigned int nCfgHeight;

#ifdef HYPERX_BRINGUP
    // Bring-up runs framed on the desktop and never changes the display mode.
    return 0;
#endif

    if (g_cUseSoftRenderer == 0)
    {
        switch (nMode)
        {
            case 1:
                DDraw::ChangeDisplayMode(1);

                return 0;

            case 0:
                if (g_pDDrawDevice != 0 && g_cD3DActive != 0)
                {
                    DDraw::ChangeDisplayMode(0);

                    g_hrRestoreResult = DDraw::RestoreAllSurfaces();
                }

                break;
        }

        return 0;
    }

    switch (nMode)
    {
        case 2:
        {
            DEVMODEA dm;

            hdc = GetWindowDC(g_hWndGame);

            nBpp    = GetDeviceCaps(hdc, BITSPIXEL);
            nWidth  = GetDeviceCaps(hdc, HORZRES);
            nHeight = GetDeviceCaps(hdc, VERTRES);

            nCfgHeight = g_dwDisplayHeight;
            nCfgWidth  = g_dwDisplayWidth;

            if (nBpp == 0x10 && nCfgWidth == nWidth && nCfgHeight == nHeight)
            {
                break;
            }

            memset(&dm, 0, sizeof(dm));

            dm.dmSize       = sizeof(dm);
            dm.dmPelsWidth  = nCfgWidth;
            dm.dmPelsHeight = nCfgHeight;
            dm.dmBitsPerPel = 0x10;
            dm.dmFields     = DM_BITSPERPEL | DM_PELSWIDTH | DM_PELSHEIGHT;

            lResult = ChangeDisplaySettingsA(&dm, 0);
            break;
        }

        case 1:
        {
            DEVMODEA dm;

            memset(&dm, 0, sizeof(dm));

            dm.dmPelsWidth     = g_nDisplayHorzRes;
            g_bSWClipRectDirty = 1;
            dm.dmSize          = sizeof(dm);
            dm.dmPelsHeight    = g_nDisplayVertRes;
            dm.dmBitsPerPel    = g_nDisplayBitsPerPixel;
            dm.dmFields        = DM_BITSPERPEL | DM_PELSWIDTH | DM_PELSHEIGHT;

            lResult = ChangeDisplaySettingsA(&dm, 0);
            break;
        }

        case 0:
        {
            DEVMODEA dm;

            hdc = GetWindowDC(g_hWndGame);

            nBpp    = GetDeviceCaps(hdc, BITSPIXEL);
            nWidth  = GetDeviceCaps(hdc, HORZRES);
            nHeight = GetDeviceCaps(hdc, VERTRES);

            const unsigned int nCfgBpp = g_dwDisplayBpp;
            nCfgHeight                 = g_dwDisplayHeight;

            if (nCfgBpp == nBpp && g_dwDisplayWidth == nWidth && nCfgHeight == nHeight)
            {
                break;
            }

            memset(&dm, 0, sizeof(dm));

            nCfgWidth = g_dwDisplayWidth;

            dm.dmSize       = sizeof(dm);
            dm.dmPelsWidth  = nCfgWidth;
            dm.dmPelsHeight = nCfgHeight;
            dm.dmBitsPerPel = nCfgBpp;
            dm.dmFields     = DM_BITSPERPEL | DM_PELSWIDTH | DM_PELSHEIGHT;

            lResult = ChangeDisplaySettingsA(&dm, 0);
            break;
        }

        default:
            break;
    }

    return lResult;
}

// FUNCTION: HYPERX 0x00493900
void Display::ApplyRenderOptions(HMENU hMenu, int bTexEnabled, int bBilinearFilter)
{
    GameApp::SetModelPath(bTexEnabled);

    Renderer::SetRenderOption(RENDER_OPTION_TEXTURED, bTexEnabled);
    Renderer::SetRenderOption(RENDER_OPTION_PERSPECTIVE, bTexEnabled);
    Renderer::SetRenderOption(RENDER_OPTION_FILTERED, bBilinearFilter);

    EnableMenuItem(hMenu, ID_RENDER_MDM_TEXTURES, bTexEnabled == 0);
    EnableMenuItem(hMenu, ID_RENDER_PERSPECTIVE, bTexEnabled == 0);
    EnableMenuItem(hMenu, ID_RENDER_BILINEAR_FILTER, bBilinearFilter == 0);

    UINT uTexCheck;

    if (bTexEnabled)
    {
        uTexCheck = MF_CHECKED;
    }
    else
    {
        uTexCheck = MF_UNCHECKED;
    }

    CheckMenuItem(hMenu, ID_RENDER_MDM_TEXTURES, uTexCheck);
    CheckMenuItem(hMenu, ID_RENDER_PERSPECTIVE, uTexCheck);

    UINT uFilterCheck;

    if (bBilinearFilter)
    {
        uFilterCheck = MF_CHECKED;
    }
    else
    {
        uFilterCheck = MF_UNCHECKED;
    }

    CheckMenuItem(hMenu, ID_RENDER_BILINEAR_FILTER, uFilterCheck);

    g_bPerspectiveTexturing = bTexEnabled;
    g_bBilinearFilter       = bBilinearFilter;
}
