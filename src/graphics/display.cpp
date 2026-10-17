#include "common/compat.h"
#include "globals.h"
#include "graphicsglobals.h"
#include "renderer.h"
#include "ui/infobarbase.h"
struct GameApp;
#include "app/window.h"
#include "graphics/graphics.h"
#include "app/menuids.h"
#include <stdlib.h>
#include "app/gameapp.h"
#include "graphics/display.h"
#include "softrenderer.h"

unsigned int g_dwDisplayWidth  = 640;
unsigned int g_dwDisplayHeight = 480;

// The back buffer resize result.
// FUNCTION: HYPERX 0x0044b500
int Display::Initialize(HWND hWnd)
{
    g_hWndGame = hWnd;

    g_nDisplayWindowID = GetWindowLongA(hWnd, GWL_ID);

    Window::ApplyFullscreenStyle(g_hWndGame, 0);

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
LONG Display::ChangeColorDepth(int)
{
    // The game runs framed on the desktop and never changes the display mode.
    return 0;
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
